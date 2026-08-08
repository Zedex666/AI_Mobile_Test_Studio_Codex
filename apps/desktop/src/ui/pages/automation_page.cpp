#include "ui/pages/automation_page.h"

#include "services/automation_artifact_service.h"
#include "ui/common/widget_helpers.h"

#include <QAbstractItemView>
#include <QBoxLayout>
#include <QDir>
#include <QDesktopServices>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QList>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QUrl>

namespace {

QLabel *makeLabel(const QString &value,
                  int size,
                  QFont::Weight weight,
                  const QString &color)
{
    auto *label = new QLabel(value);
    label->setFont(ui::appFont(size, weight));
    label->setStyleSheet(QStringLiteral("color:%1;").arg(color));
    return label;
}

QPushButton *makeToolbarButton(const QString &label, const QString &tooltip)
{
    auto *button = new QPushButton(label);
    button->setObjectName(QStringLiteral("AutomationToolbarButton"));
    button->setToolTip(tooltip);
    button->setCursor(Qt::PointingHandCursor);
    button->setFont(ui::appFont(9, QFont::DemiBold));
    button->setMinimumHeight(34);
    return button;
}

QString categoryLabel(const QString &categoryId)
{
    return categoryId == QStringLiteral("scripts") ? ui::text("脚本") : ui::text("报告");
}

QString formatBytes(qint64 bytes)
{
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(QString::number(bytes / 1024.0, 'f', 1));
    }
    return QStringLiteral("%1 MB").arg(QString::number(bytes / (1024.0 * 1024.0), 'f', 1));
}

} // namespace

AutomationPage::AutomationPage(AutomationArtifactService *service, QWidget *parent)
    : QWidget(parent)
    , m_service(service)
{
    setObjectName(QStringLiteral("AutomationPage"));
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(18, 16, 18, 16);
    pageLayout->setSpacing(12);

    auto *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(10);
    titleRow->addWidget(makeLabel(ui::text("自动化"), 16, QFont::DemiBold, "#111827"));
    titleRow->addStretch();
    m_rootPath = makeLabel(QString(), 8, QFont::Normal, "#7a8698");
    m_rootPath->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    m_rootPath->setTextInteractionFlags(Qt::TextSelectableByMouse);
    titleRow->addWidget(m_rootPath, 1);
    m_refreshButton = makeToolbarButton(ui::text("刷新"), ui::text("刷新自动化产物"));
    m_openButton = makeToolbarButton(ui::text("打开/运行"), ui::text("在默认浏览器中打开选中的 HTML 文件"));
    m_openButton->setObjectName(QStringLiteral("AutomationOpenButton"));
    titleRow->addWidget(m_refreshButton);
    titleRow->addWidget(m_openButton);
    pageLayout->addLayout(titleRow);

    auto *toolbar = new QWidget;
    toolbar->setObjectName(QStringLiteral("AutomationToolbar"));
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(12, 8, 12, 8);
    toolbarLayout->setSpacing(8);
    toolbarLayout->addWidget(makeLabel(ui::text("HTML 产物"), 10, QFont::DemiBold, "#435066"));
    m_searchInput = new QLineEdit;
    m_searchInput->setObjectName(QStringLiteral("AutomationSearchInput"));
    m_searchInput->setPlaceholderText(ui::text("搜索脚本或报告"));
    m_searchInput->setClearButtonEnabled(true);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(m_searchInput);
    pageLayout->addWidget(toolbar);

    auto *content = ui::makePanel(QStringLiteral("AutomationContent"));
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    m_table = new QTableWidget;
    m_table->setObjectName(QStringLiteral("AutomationTable"));
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({ui::text("名称"),
                                        ui::text("类型"),
                                        ui::text("相对路径"),
                                        ui::text("大小"),
                                        ui::text("更新时间")});
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setShowGrid(false);
    m_table->verticalHeader()->setVisible(false);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    m_table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    contentLayout->addWidget(m_table, 1);

    m_emptyState = makeLabel(ui::text("暂无 HTML 产物"), 11, QFont::DemiBold, "#7a8698");
    m_emptyState->setAlignment(Qt::AlignCenter);
    m_emptyState->setMinimumHeight(120);
    contentLayout->addWidget(m_emptyState);
    pageLayout->addWidget(content, 1);

    m_status = makeLabel(QString(), 8, QFont::Normal, "#7a8698");
    pageLayout->addWidget(m_status);

    connect(m_refreshButton, &QPushButton::clicked, this, &AutomationPage::refreshArtifacts);
    connect(m_openButton, &QPushButton::clicked, this, &AutomationPage::openSelectedArtifact);
    connect(m_searchInput, &QLineEdit::textChanged, this, &AutomationPage::applyFilter);
    connect(m_table,
            &QTableWidget::itemSelectionChanged,
            this,
            &AutomationPage::updateSelection);
    connect(m_table,
            &QTableWidget::cellDoubleClicked,
            this,
            [this](int, int) { openSelectedArtifact(); });
    if (m_service != nullptr) {
        connect(m_service,
                &AutomationArtifactService::artifactsChanged,
                this,
                &AutomationPage::rebuildArtifactTable);
        connect(m_service,
                &AutomationArtifactService::refreshFailed,
                this,
                &AutomationPage::showRefreshError);
        m_rootPath->setText(m_service->automationDirectory());
        m_rootPath->setToolTip(m_service->automationDirectory());
    }
    updateControls();
}

void AutomationPage::activate()
{
    refreshArtifacts();
}

void AutomationPage::refreshArtifacts()
{
    if (m_service == nullptr) {
        return;
    }
    m_service->refresh();
    m_rootPath->setText(QDir::toNativeSeparators(m_service->automationDirectory()));
    m_rootPath->setToolTip(m_service->automationDirectory());
}

void AutomationPage::updateSelection()
{
    updateControls();
}

void AutomationPage::openSelectedArtifact()
{
    const QString path = selectedArtifactPath();
    if (path.isEmpty()) {
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(path))) {
        showRefreshError(ui::text("无法调用默认浏览器打开文件"));
        return;
    }
    m_status->setStyleSheet(QStringLiteral("color:#459b47;"));
    m_status->setText(ui::text("已在默认浏览器中打开：%1").arg(QFileInfo(path).fileName()));
}

void AutomationPage::applyFilter()
{
    const QString query = m_searchInput->text().trimmed();
    for (int row = 0; row < m_table->rowCount(); ++row) {
        const QString name = m_table->item(row, 0)->text();
        const QString path = m_table->item(row, 2)->text();
        const bool visible = query.isEmpty()
            || name.contains(query, Qt::CaseInsensitive)
            || path.contains(query, Qt::CaseInsensitive);
        m_table->setRowHidden(row, !visible);
    }
    updateControls();
}

void AutomationPage::showRefreshError(const QString &message)
{
    m_status->setStyleSheet(QStringLiteral("color:#d45b5b;"));
    m_status->setText(message);
}

void AutomationPage::rebuildArtifactTable()
{
    if (m_service == nullptr) {
        return;
    }
    const QString selectedPath = selectedArtifactPath();
    m_table->setRowCount(0);
    for (const AutomationArtifact &artifact : m_service->artifacts()) {
        const int row = m_table->rowCount();
        m_table->insertRow(row);
        auto *name = new QTableWidgetItem(artifact.name);
        name->setData(Qt::UserRole, artifact.absolutePath);
        name->setToolTip(artifact.absolutePath);
        m_table->setItem(row, 0, name);
        m_table->setItem(row, 1, new QTableWidgetItem(categoryLabel(artifact.categoryId)));
        m_table->setItem(row, 2, new QTableWidgetItem(artifact.relativePath));
        m_table->setItem(row, 3, new QTableWidgetItem(formatBytes(artifact.size)));
        m_table->setItem(row,
                         4,
                         new QTableWidgetItem(artifact.modifiedAt.toLocalTime()
                                                  .toString(QStringLiteral("yyyy-MM-dd HH:mm"))));
    }
    m_emptyState->setVisible(m_table->rowCount() == 0);
    m_status->setStyleSheet(QStringLiteral("color:#7a8698;"));
    m_status->setText(ui::text("共 %1 个 HTML 产物").arg(m_service->artifacts().size()));
    applyFilter();
    if (!selectedPath.isEmpty()) {
        for (int row = 0; row < m_table->rowCount(); ++row) {
            if (m_table->item(row, 0)->data(Qt::UserRole).toString() == selectedPath) {
                m_table->selectRow(row);
                break;
            }
        }
    }
    updateControls();
}

QString AutomationPage::selectedArtifactPath() const
{
    const QList<QTableWidgetItem *> selected = m_table->selectedItems();
    if (selected.isEmpty()) {
        return QString();
    }
    return selected.first()->data(Qt::UserRole).toString();
}

void AutomationPage::updateControls()
{
    const bool selected = !selectedArtifactPath().isEmpty();
    m_openButton->setEnabled(selected);
}
