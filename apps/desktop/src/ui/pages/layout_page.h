#ifndef AI_MOBILE_TEST_STUDIO_LAYOUT_PAGE_H
#define AI_MOBILE_TEST_STUDIO_LAYOUT_PAGE_H

#include <QJsonObject>
#include <QVector>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QHideEvent;
class QLabel;
class QLineEdit;
class QListWidget;
class QNetworkAccessManager;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QShowEvent;
class QTableWidget;
class QTabWidget;
class QTreeWidget;
class QWidget;

class LayoutPage : public QWidget
{
    Q_OBJECT

public:
    explicit LayoutPage(QWidget *parent = nullptr);

    void setDeviceConnected(bool connected, const QString &serial);
    void preload();
    void finishPreload();

signals:
    void preloadReady();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    struct CapabilityRow {
        QWidget *container = nullptr;
        QLineEdit *name = nullptr;
        QComboBox *type = nullptr;
        QWidget *value = nullptr;
        QCheckBox *enabled = nullptr;
        QPushButton *remove = nullptr;
    };

    void buildSessionBuilder();
    void buildInspector();
    QWidget *buildServerTab();
    QWidget *buildCloudProvidersTab();
    QWidget *buildCapabilityBuilderTab();
    QWidget *buildSavedCapabilitiesTab();
    QWidget *buildAttachSessionTab();
    QWidget *buildSourceTab();
    QWidget *buildCommandsTab();
    QWidget *buildGesturesTab();
    QWidget *buildRecorderTab();
    QWidget *buildSessionInfoTab();

    CapabilityRow addCapabilityRow(const QString &name,
                                   const QString &type,
                                   const QVariant &value,
                                   bool enabled = true);
    void rebuildCapabilityValue(CapabilityRow &row, const QVariant &value = {});
    void removeCapability(CapabilityRow *row);
    void updateCapabilityJson();
    QJsonObject capabilityObject() const;
    void saveCapabilitiesAs();
    void importCapabilities();
    void startSession();
    void returnToBuilder();
    void refreshInspectorSource();
    void setInspectorStatus(const QString &message, const QString &color = QStringLiteral("#596579"));
    void postSessionToAppium();
    void checkWebInspectorPreload();
    void updateWebInspectorActivity(bool active);

    QStackedWidget *m_modeStack = nullptr;
    QWidget *m_sessionBuilder = nullptr;
    QWidget *m_sessionInspector = nullptr;
    QTabWidget *m_builderTabs = nullptr;
    QTabWidget *m_serverTabs = nullptr;
    QTabWidget *m_inspectorTabs = nullptr;
    QStackedWidget *m_builderCapabilityStack = nullptr;
    QPlainTextEdit *m_capabilityJson = nullptr;
    QLineEdit *m_remoteHost = nullptr;
    QLineEdit *m_remotePort = nullptr;
    QLineEdit *m_remotePath = nullptr;
    QCheckBox *m_ssl = nullptr;
    QCheckBox *m_allowUnauthorized = nullptr;
    QCheckBox *m_useProxy = nullptr;
    QLineEdit *m_proxy = nullptr;
    QCheckBox *m_autoPrefixes = nullptr;
    QLineEdit *m_attachSessionId = nullptr;
    QLabel *m_builderStatus = nullptr;
    QLabel *m_inspectorStatus = nullptr;
    QLabel *m_deviceLabel = nullptr;
    QLabel *m_sessionIdLabel = nullptr;
    QLineEdit *m_sourceSearch = nullptr;
    QTreeWidget *m_sourceTree = nullptr;
    QTableWidget *m_savedTable = nullptr;
    QTableWidget *m_gestureTable = nullptr;
    QTableWidget *m_recorderTable = nullptr;
    QVector<CapabilityRow> m_capabilityRows;
    QVector<QJsonObject> m_savedCapabilitySets;
    QNetworkAccessManager *m_network = nullptr;
    QWidget *m_webInspector = nullptr;
    QString m_deviceSerial;
    QString m_sessionId;
    bool m_deviceConnected = false;
    bool m_recording = false;
    bool m_webInspectorPreloadRequested = false;
    bool m_webInspectorPreloadReady = false;
};

#endif // AI_MOBILE_TEST_STUDIO_LAYOUT_PAGE_H
