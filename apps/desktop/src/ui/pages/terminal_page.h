#ifndef AI_MOBILE_TEST_STUDIO_TERMINAL_PAGE_H
#define AI_MOBILE_TEST_STUDIO_TERMINAL_PAGE_H

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QWidget>

class QStackedWidget;
class QHideEvent;
class QShowEvent;
class QTabBar;
class QToolButton;
class TerminalView;

class TerminalPage : public QWidget
{
    Q_OBJECT

public:
    explicit TerminalPage(QWidget *parent = nullptr);

    void setDeviceConnected(bool connected, const QString &serial);
    void activate();
    void preloadOpenCode();
    void finishBackgroundPreload();

public slots:
    void handleSessionStarted(const QString &sessionId);
    void handleSessionOutput(const QString &sessionId, const QByteArray &data);
    void handleSessionFailed(const QString &sessionId, const QString &message);
    void handleSessionClosed(const QString &sessionId, bool expected, const QString &message);

signals:
    void sessionCreateRequested(const QString &sessionId, const QString &kindId);
    void sessionWriteRequested(const QString &sessionId, const QByteArray &data);
    void sessionResizeRequested(const QString &sessionId, int columns, int rows);
    void sessionRestartRequested(const QString &sessionId);
    void sessionCloseRequested(const QString &sessionId);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void addTerminal(const QString &kindId);
    void showNewTerminalMenu(QWidget *anchor);
    void closeTerminal(int index);
    void selectTerminal(int index);
    void runShortcut(const QByteArray &command);
    QString currentSessionId() const;
    QString kindForSession(const QString &sessionId) const;
    TerminalView *viewForSession(const QString &sessionId) const;
    void updateControls();

    bool m_connected = false;
    QString m_serial;
    int m_nextTerminalNumber = 1;
    int m_nextOpenCodeNumber = 1;
    QString m_backgroundRenderSessionId;
    bool m_backgroundPreloading = false;
    QTabBar *m_tabBar = nullptr;
    QToolButton *m_addButton = nullptr;
    QToolButton *m_shortcutsButton = nullptr;
    QStackedWidget *m_terminalStack = nullptr;
    QHash<QString, TerminalView *> m_views;
    QHash<QString, QString> m_sessionKinds;
};

#endif // AI_MOBILE_TEST_STUDIO_TERMINAL_PAGE_H
