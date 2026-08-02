#include "ui/pages/terminal_bridge.h"

#include "ui/common/app_preferences.h"

#include <QApplication>
#include <QClipboard>

#include <utility>

namespace {

constexpr qsizetype kMaximumPendingOutput = 4 * 1024 * 1024;
constexpr qsizetype kMaximumOutputPerFrame = 32 * 1024;
constexpr qsizetype kOutputCompactionThreshold = 512 * 1024;
constexpr int kOutputFrameIntervalMs = 8;

QString terminalFontFamily(ui::AppLanguage language)
{
    return language == ui::AppLanguage::English
        ? QStringLiteral("\"AI JetBrains Mono\", monospace")
        : QStringLiteral("\"AI LXGW WenKai\", sans-serif");
}

} // namespace

TerminalBridge::TerminalBridge(QObject *parent)
    : QObject(parent)
    , m_fontFamily(terminalFontFamily(ui::AppPreferences::instance().language()))
{
    m_outputFlushTimer.setSingleShot(true);
    connect(&m_outputFlushTimer, &QTimer::timeout, this, &TerminalBridge::flushOutput);
    connect(&ui::AppPreferences::instance(),
            &ui::AppPreferences::languageChanged,
            this,
            [this](ui::AppLanguage language) {
                const QString family = terminalFontFamily(language);
                if (m_fontFamily == family) {
                    return;
                }
                m_fontFamily = family;
                emit fontFamilyChanged(m_fontFamily);
            });
}

void TerminalBridge::sendOutput(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }
    if (m_pendingOutputOffset >= kOutputCompactionThreshold
        || (m_pendingOutputOffset > 0
            && m_pendingOutput.size() - m_pendingOutputOffset + data.size()
                   > kMaximumPendingOutput)) {
        m_pendingOutput.remove(0, m_pendingOutputOffset);
        m_pendingOutputOffset = 0;
    }
    m_pendingOutput.append(data);
    const qsizetype pendingSize = m_pendingOutput.size() - m_pendingOutputOffset;
    if (pendingSize > kMaximumPendingOutput) {
        m_pendingOutputOffset += pendingSize - kMaximumPendingOutput;
    }
    if (m_frontendReady && m_deliveryEnabled && !m_outputInFlight
        && !m_outputFlushTimer.isActive()) {
        m_outputFlushTimer.start(kOutputFrameIntervalMs);
    }
}

void TerminalBridge::sendStatus(const QString &message)
{
    if (!m_frontendReady) {
        m_pendingStatus.append(message);
        return;
    }
    emit statusMessage(message);
}

void TerminalBridge::clear()
{
    m_pendingOutput.clear();
    m_pendingOutputOffset = 0;
    m_pendingStatus.clear();
    m_outputFlushTimer.stop();
    m_outputInFlight = false;
    if (m_frontendReady) {
        emit clearRequested();
    }
}

void TerminalBridge::focus()
{
    if (m_frontendReady) {
        emit focusRequested();
    }
}

void TerminalBridge::setSessionReady(bool ready)
{
    m_sessionReady = ready;
    if (m_frontendReady) {
        emit sessionReadyChanged(ready);
    }
}

void TerminalBridge::setDeliveryEnabled(bool enabled)
{
    if (m_deliveryEnabled == enabled) {
        return;
    }
    m_deliveryEnabled = enabled;
    if (!enabled) {
        m_outputFlushTimer.stop();
        return;
    }
    if (m_frontendReady && !m_outputInFlight
        && m_pendingOutput.size() > m_pendingOutputOffset) {
        m_outputFlushTimer.start(0);
    }
}

QString TerminalBridge::fontFamily() const
{
    return m_fontFamily;
}

void TerminalBridge::frontendReady(int columns, int rows)
{
    if (m_frontendReady) {
        return;
    }
    m_frontendReady = true;
    emit sessionReadyChanged(m_sessionReady);
    if (m_deliveryEnabled && m_pendingOutput.size() > m_pendingOutputOffset
        && !m_outputInFlight && !m_outputFlushTimer.isActive()) {
        m_outputFlushTimer.start(0);
    }
    for (const QString &message : std::as_const(m_pendingStatus)) {
        emit statusMessage(message);
    }
    m_pendingStatus.clear();
    resizeTerminal(columns, rows);
    emit ready();
}

void TerminalBridge::outputConsumed()
{
    if (!m_outputInFlight) {
        return;
    }
    m_outputInFlight = false;
    if (m_pendingOutputOffset >= m_pendingOutput.size()) {
        m_pendingOutput.clear();
        m_pendingOutputOffset = 0;
    }
    if (m_deliveryEnabled && m_pendingOutput.size() > m_pendingOutputOffset
        && !m_outputFlushTimer.isActive()) {
        m_outputFlushTimer.start(kOutputFrameIntervalMs);
    }
}

void TerminalBridge::flushOutput()
{
    if (!m_frontendReady || !m_deliveryEnabled || m_outputInFlight
        || m_pendingOutput.size() <= m_pendingOutputOffset) {
        return;
    }

    const qsizetype chunkSize = qMin(kMaximumOutputPerFrame,
                                     m_pendingOutput.size() - m_pendingOutputOffset);
    const QByteArray chunk = m_pendingOutput.mid(m_pendingOutputOffset, chunkSize);
    m_pendingOutputOffset += chunkSize;
    m_outputInFlight = true;
    emit outputData(QString::fromLatin1(chunk.toBase64()));
}

void TerminalBridge::writeInput(const QString &data)
{
    if (m_sessionReady && writeRequested && !data.isEmpty()) {
        writeRequested(data.toUtf8());
    }
}

void TerminalBridge::resizeTerminal(int columns, int rows)
{
    if (m_sessionReady && resizeRequested && columns > 0 && rows > 0) {
        resizeRequested(columns, rows);
    }
}

void TerminalBridge::copyText(const QString &text)
{
    if (!text.isEmpty()) {
        QApplication::clipboard()->setText(text);
    }
}

void TerminalBridge::pasteClipboard()
{
    if (!m_sessionReady) {
        return;
    }
    const QString text = QApplication::clipboard()->text();
    if (!text.isEmpty()) {
        emit pasteData(text);
    }
}
