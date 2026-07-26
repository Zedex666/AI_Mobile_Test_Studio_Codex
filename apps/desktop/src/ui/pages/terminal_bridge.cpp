#include "ui/pages/terminal_bridge.h"

#include <QApplication>
#include <QClipboard>

#include <utility>

namespace {

constexpr qsizetype kMaximumPendingOutput = 4 * 1024 * 1024;

} // namespace

TerminalBridge::TerminalBridge(QObject *parent)
    : QObject(parent)
{
}

void TerminalBridge::sendOutput(const QByteArray &data)
{
    if (data.isEmpty()) {
        return;
    }
    if (!m_frontendReady) {
        m_pendingOutput.append(data);
        if (m_pendingOutput.size() > kMaximumPendingOutput) {
            m_pendingOutput.remove(0, m_pendingOutput.size() - kMaximumPendingOutput);
        }
        return;
    }
    emit outputData(QString::fromLatin1(data.toBase64()));
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
    m_pendingStatus.clear();
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

void TerminalBridge::frontendReady(int columns, int rows)
{
    if (m_frontendReady) {
        return;
    }
    m_frontendReady = true;
    emit sessionReadyChanged(m_sessionReady);
    if (!m_pendingOutput.isEmpty()) {
        emit outputData(QString::fromLatin1(m_pendingOutput.toBase64()));
        m_pendingOutput.clear();
    }
    for (const QString &message : std::as_const(m_pendingStatus)) {
        emit statusMessage(message);
    }
    m_pendingStatus.clear();
    resizeTerminal(columns, rows);
    emit ready();
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
