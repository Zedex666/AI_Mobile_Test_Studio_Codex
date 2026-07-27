#ifndef AI_MOBILE_TEST_STUDIO_LOGCAT_PAGE_H
#define AI_MOBILE_TEST_STUDIO_LOGCAT_PAGE_H

#include "services/logcat_service.h"

#include <QVector>
#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QStackedWidget;
class QTableView;
class QTimer;
class QToolButton;
class LogcatModel;
class LogcatFilterModel;

class LogcatPage : public QWidget
{
    Q_OBJECT

public:
    explicit LogcatPage(QWidget *parent = nullptr);

public slots:
    void setDeviceConnected(bool connected, const QString &serial);
    void appendEntry(const LogcatEntry &entry);
    void setStreamRunning(bool running);
    void showStreamError(const QString &message);

signals:
    void restartRequested();

private:
    void flushEntries();
    void updateFilters();
    void updateViewMode();
    void updateStatus();
    void saveEntries();
    void clearEntries();
    void toggleSoftWrap();
    void togglePaused();

    LogcatModel *m_model = nullptr;
    LogcatFilterModel *m_filterModel = nullptr;
    QTableView *m_table = nullptr;
    QStackedWidget *m_contentStack = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QLabel *m_statusLabel = nullptr;
    QComboBox *m_viewCombo = nullptr;
    QComboBox *m_priorityCombo = nullptr;
    QLineEdit *m_packageInput = nullptr;
    QLineEdit *m_tagInput = nullptr;
    QToolButton *m_wrapButton = nullptr;
    QToolButton *m_pauseButton = nullptr;
    QVector<LogcatEntry> m_pendingEntries;
    QTimer *m_flushTimer = nullptr;
    bool m_connected = false;
    bool m_streamRunning = false;
    bool m_paused = false;
    bool m_softWrap = false;
    QString m_serial;
};

#endif // AI_MOBILE_TEST_STUDIO_LOGCAT_PAGE_H
