#ifndef AI_MOBILE_TEST_STUDIO_AUTOMATION_PAGE_H
#define AI_MOBILE_TEST_STUDIO_AUTOMATION_PAGE_H

#include <QWidget>

class QLabel;
class QLineEdit;
class QPushButton;
class QTableWidget;
class AutomationArtifactService;

class AutomationPage : public QWidget
{
    Q_OBJECT

public:
    explicit AutomationPage(AutomationArtifactService *service,
                            QWidget *parent = nullptr);

public slots:
    void activate();

private slots:
    void refreshArtifacts();
    void updateSelection();
    void openSelectedArtifact();
    void applyFilter();
    void showRefreshError(const QString &message);

private:
    void rebuildArtifactTable();
    QString selectedArtifactPath() const;
    void updateControls();

    AutomationArtifactService *m_service = nullptr;
    QLabel *m_rootPath = nullptr;
    QLabel *m_status = nullptr;
    QLabel *m_emptyState = nullptr;
    QLineEdit *m_searchInput = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_openButton = nullptr;
    QTableWidget *m_table = nullptr;
};

#endif // AI_MOBILE_TEST_STUDIO_AUTOMATION_PAGE_H
