#ifndef AI_MOBILE_TEST_STUDIO_OVERVIEW_PAGE_H
#define AI_MOBILE_TEST_STUDIO_OVERVIEW_PAGE_H

#include "services/overview_service.h"

#include <QHash>
#include <QWidget>

class QLabel;
class QFrame;
class QToolButton;

class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = nullptr);

    void activate();

public slots:
    void setDeviceConnected(bool connected, const QString &serial);
    void setLoading(bool loading);
    void setOverview(const DeviceOverview &overview);
    void showError(const QString &message);

signals:
    void refreshRequested();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QLabel *addInfoItem(class QGridLayout *layout,
                        int row,
                        int column,
                        const QString &key,
                        const QString &icon,
                        const QString &title);
    void resetValues();
    void setValue(const QString &key, const QString &value);

    QHash<QString, QLabel *> m_values;
    QFrame *m_card = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QToolButton *m_refreshButton = nullptr;
    bool m_connected = false;
    bool m_hasData = false;
    QString m_serial;
};

#endif // AI_MOBILE_TEST_STUDIO_OVERVIEW_PAGE_H
