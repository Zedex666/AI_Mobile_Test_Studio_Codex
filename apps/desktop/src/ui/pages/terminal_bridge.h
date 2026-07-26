#ifndef AI_MOBILE_TEST_STUDIO_TERMINAL_BRIDGE_H
#define AI_MOBILE_TEST_STUDIO_TERMINAL_BRIDGE_H

#include <QByteArray>
#include <QObject>
#include <QStringList>

#include <functional>

class TerminalBridge : public QObject
{
    Q_OBJECT

public:
    explicit TerminalBridge(QObject *parent = nullptr);

    std::function<void(const QByteArray &)> writeRequested;
    std::function<void(int, int)> resizeRequested;

    void sendOutput(const QByteArray &data);
    void sendStatus(const QString &message);
    void clear();
    void focus();
    void setSessionReady(bool ready);

public slots:
    void frontendReady(int columns, int rows);
    void writeInput(const QString &data);
    void resizeTerminal(int columns, int rows);
    void copyText(const QString &text);
    void pasteClipboard();

signals:
    void outputData(const QString &base64Data);
    void statusMessage(const QString &message);
    void clearRequested();
    void focusRequested();
    void pasteData(const QString &text);
    void sessionReadyChanged(bool ready);
    void ready();

private:
    QByteArray m_pendingOutput;
    QStringList m_pendingStatus;
    bool m_frontendReady = false;
    bool m_sessionReady = false;
};

#endif // AI_MOBILE_TEST_STUDIO_TERMINAL_BRIDGE_H
