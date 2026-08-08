#include "services/studio_control_protocol.h"

#include <QJsonDocument>
#include <QJsonParseError>

namespace studio_control {

namespace {

constexpr qsizetype kHeaderBytes = 4;

void appendLittleEndian32(QByteArray *data, quint32 value)
{
    data->append(static_cast<char>(value & 0xff));
    data->append(static_cast<char>((value >> 8) & 0xff));
    data->append(static_cast<char>((value >> 16) & 0xff));
    data->append(static_cast<char>((value >> 24) & 0xff));
}

quint32 readLittleEndian32(const QByteArray &data)
{
    return static_cast<quint32>(static_cast<unsigned char>(data.at(0)))
        | (static_cast<quint32>(static_cast<unsigned char>(data.at(1))) << 8)
        | (static_cast<quint32>(static_cast<unsigned char>(data.at(2))) << 16)
        | (static_cast<quint32>(static_cast<unsigned char>(data.at(3))) << 24);
}

} // namespace

QByteArray encodeFrame(const QJsonObject &message)
{
    const QByteArray payload = QJsonDocument(message).toJson(QJsonDocument::Compact);
    QByteArray frame;
    frame.reserve(kHeaderBytes + payload.size());
    appendLittleEndian32(&frame, static_cast<quint32>(payload.size()));
    frame.append(payload);
    return frame;
}

DecodeResult decodeNextFrame(QByteArray *buffer, QJsonObject *message, QString *error)
{
    if (buffer == nullptr || message == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("Invalid decoder arguments.");
        }
        return DecodeResult::Invalid;
    }
    if (buffer->size() < kHeaderBytes) {
        return DecodeResult::Incomplete;
    }

    const quint32 payloadSize = readLittleEndian32(*buffer);
    if (payloadSize == 0 || payloadSize > static_cast<quint32>(kMaximumFrameBytes)) {
        if (error != nullptr) {
            *error = QStringLiteral("Frame size is outside the allowed range.");
        }
        buffer->clear();
        return DecodeResult::Invalid;
    }
    if (buffer->size() < kHeaderBytes + static_cast<qsizetype>(payloadSize)) {
        return DecodeResult::Incomplete;
    }

    const QByteArray payload = buffer->mid(kHeaderBytes, payloadSize);
    buffer->remove(0, kHeaderBytes + payloadSize);
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error != nullptr) {
            *error = parseError.error == QJsonParseError::NoError
                ? QStringLiteral("JSON-RPC payload must be an object.")
                : parseError.errorString();
        }
        return DecodeResult::Invalid;
    }

    *message = document.object();
    return DecodeResult::Complete;
}

} // namespace studio_control
