#ifndef AI_MOBILE_TEST_STUDIO_TERMINAL_BRIDGE_H
#define AI_MOBILE_TEST_STUDIO_TERMINAL_BRIDGE_H

#include <QByteArray>
#include <QObject>
#include <QStringList>
#include <QTimer>

#include <functional>

class TerminalBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString fontFamily READ fontFamily NOTIFY fontFamilyChanged)

public:
    explicit TerminalBridge(QObject *parent = nullptr);

    std::function<void(const QByteArray &)> writeRequested;
    std::function<void(int, int)> resizeRequested;

    void sendOutput(const QByteArray &data);
    void sendStatus(const QString &message);
    void clear();
    void focus();
    void setSessionReady(bool ready);
    void setDeliveryEnabled(bool enabled);
    QString fontFamily() const;

public slots:
    void frontendReady(int columns, int rows);
    void outputConsumed();
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
    void fontFamilyChanged(const QString &fontFamily);
    void ready();

private:
    void flushOutput();

    QByteArray m_pendingOutput;
    qsizetype m_pendingOutputOffset = 0;
    QStringList m_pendingStatus;
    QTimer m_outputFlushTimer;
    QString m_fontFamily;
    bool m_frontendReady = false;
    bool m_sessionReady = false;
    bool m_deliveryEnabled = true;
    bool m_outputInFlight = false;
};

#endif // AI_MOBILE_TEST_STUDIO_TERMINAL_BRIDGE_H
