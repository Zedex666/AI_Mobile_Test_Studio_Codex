#include "services/adb_shell_session.h"

#include <QTimer>

#include <utility>

namespace {

constexpr quint16 kDefaultAdbServerPort = 5037;
constexpr quint32 kMaximumShellPacketSize = 16 * 1024 * 1024;

QByteArray encodeAdbRequest(const QByteArray &request)
{
    return QByteArray::number(request.size(), 16).rightJustified(4, '0').toUpper() + request;
}

QByteArray encodeShellPacket(quint8 id, const QByteArray &payload)
{
    const quint32 size = static_cast<quint32>(payload.size());
    QByteArray packet;
    packet.reserve(5 + payload.size());
    packet.append(static_cast<char>(id));
    packet.append(static_cast<char>(size & 0xff));
    packet.append(static_cast<char>((size >> 8) & 0xff));
    packet.append(static_cast<char>((size >> 16) & 0xff));
    packet.append(static_cast<char>((size >> 24) & 0xff));
    packet.append(payload);
    return packet;
}

quint32 readLittleEndian32(const QByteArray &data, int offset)
{
    return static_cast<quint32>(static_cast<unsigned char>(data.at(offset)))
        | (static_cast<quint32>(static_cast<unsigned char>(data.at(offset + 1))) << 8)
        | (static_cast<quint32>(static_cast<unsigned char>(data.at(offset + 2))) << 16)
        | (static_cast<quint32>(static_cast<unsigned char>(data.at(offset + 3))) << 24);
}

quint16 adbServerPort()
{
    bool valid = false;
    const int configured = qEnvironmentVariableIntValue("ANDROID_ADB_SERVER_PORT", &valid);
    return valid && configured > 0 && configured <= 65535
        ? static_cast<quint16>(configured)
        : kDefaultAdbServerPort;
}

QString adbServerHost()
{
    const QString configured = qEnvironmentVariable("ANDROID_ADB_SERVER_HOST");
    return configured.isEmpty() ? QStringLiteral("127.0.0.1") : configured;
}

} // namespace

AdbShellSession::AdbShellSession(QString deviceSerial, QObject *parent)
    : TerminalSession(TerminalSessionKind::AdbShell, parent)
    , m_deviceSerial(std::move(deviceSerial))
{
    connect(&m_socket, &QTcpSocket::connected, this, [this] {
        m_state = State::SelectingTransport;
        m_socket.write(
            encodeAdbRequest(QByteArrayLiteral("host:transport:") + m_deviceSerial.toUtf8()));
    });
    connect(&m_socket, &QTcpSocket::readyRead, this, [this] {
        m_buffer += m_socket.readAll();
        processIncomingData();
    });
    connect(&m_socket, &QTcpSocket::disconnected, this, [this] {
        if (!m_finished && !m_reconnecting) {
            finish(m_started, QStringLiteral("ADB shell 连接已断开"));
        }
    });
    connect(&m_socket,
            &QTcpSocket::errorOccurred,
            this,
            [this](QAbstractSocket::SocketError) {
                if (!m_finished && !m_reconnecting) {
                    finish(m_started,
                           QStringLiteral("无法连接 ADB server：%1")
                               .arg(m_socket.errorString()));
                }
            });
}

void AdbShellSession::start()
{
    m_socket.connectToHost(adbServerHost(), adbServerPort());
}

void AdbShellSession::write(const QByteArray &data)
{
    if (m_state != State::Streaming || data.isEmpty()) {
        return;
    }
    m_socket.write(m_useShellV2 ? encodeShellPacket(0, data) : data);
}

void AdbShellSession::resize(int columns, int rows)
{
    if (m_state != State::Streaming || !m_useShellV2 || columns <= 0 || rows <= 0) {
        return;
    }
    QByteArray size = QByteArray::number(rows) + 'x' + QByteArray::number(columns)
        + QByteArrayLiteral(",0x0");
    size.append('\0');
    m_socket.write(encodeShellPacket(5, size));
}

void AdbShellSession::stop()
{
    m_finished = true;
    m_socket.abort();
}

void AdbShellSession::processIncomingData()
{
    while (!m_finished) {
        if (m_state == State::SelectingTransport || m_state == State::OpeningShell) {
            if (!processServiceResponse()) {
                return;
            }
            continue;
        }

        if (m_state != State::Streaming || m_buffer.isEmpty()) {
            return;
        }

        if (!m_useShellV2) {
            const QByteArray data = std::exchange(m_buffer, QByteArray());
            emit outputReady(data);
            return;
        }

        if (m_buffer.size() < 5) {
            return;
        }
        const quint8 packetId = static_cast<quint8>(m_buffer.at(0));
        const quint32 packetSize = readLittleEndian32(m_buffer, 1);
        if (packetSize > kMaximumShellPacketSize) {
            finish(true, QStringLiteral("ADB shell 返回了无效的数据包"));
            return;
        }
        if (m_buffer.size() < 5 + static_cast<int>(packetSize)) {
            return;
        }

        const QByteArray payload = m_buffer.mid(5, static_cast<int>(packetSize));
        m_buffer.remove(0, 5 + static_cast<int>(packetSize));
        if (packetId == 1 || packetId == 2) {
            emit outputReady(payload);
        } else if (packetId == 3) {
            finish(true, QStringLiteral("远程 shell 已退出"));
            return;
        }
    }
}

bool AdbShellSession::processServiceResponse()
{
    if (m_buffer.size() < 4) {
        return false;
    }

    const QByteArray status = m_buffer.left(4);
    if (status == QByteArrayLiteral("OKAY")) {
        m_buffer.remove(0, 4);
        if (m_state == State::SelectingTransport) {
            m_state = State::OpeningShell;
            m_socket.write(encodeAdbRequest(m_useShellV2 ? QByteArrayLiteral("shell,v2:")
                                                         : QByteArrayLiteral("shell:")));
            return true;
        }

        m_state = State::Streaming;
        m_started = true;
        emit started();
        return true;
    }

    if (status != QByteArrayLiteral("FAIL")) {
        finish(false, QStringLiteral("ADB server 返回了无效响应"));
        return false;
    }
    if (m_buffer.size() < 8) {
        return false;
    }

    bool validLength = false;
    const int messageSize = m_buffer.mid(4, 4).toInt(&validLength, 16);
    if (!validLength || messageSize < 0) {
        finish(false, QStringLiteral("ADB server 返回了无效错误信息"));
        return false;
    }
    if (m_buffer.size() < 8 + messageSize) {
        return false;
    }

    const QString detail = QString::fromUtf8(m_buffer.mid(8, messageSize));
    m_buffer.remove(0, 8 + messageSize);
    if (m_state == State::OpeningShell && m_useShellV2) {
        retryWithLegacyShell();
        return false;
    }

    finish(false, detail.isEmpty() ? QStringLiteral("ADB shell 创建失败") : detail);
    return false;
}

void AdbShellSession::retryWithLegacyShell()
{
    m_reconnecting = true;
    m_useShellV2 = false;
    m_buffer.clear();
    m_state = State::Connecting;
    m_socket.abort();
    QTimer::singleShot(0, this, [this] {
        if (m_finished) {
            return;
        }
        m_reconnecting = false;
        m_socket.connectToHost(adbServerHost(), adbServerPort());
    });
}

void AdbShellSession::finish(bool wasStarted, const QString &message)
{
    if (m_finished) {
        return;
    }
    m_finished = true;
    m_socket.abort();
    emit exited(wasStarted, message);
}
