#include "ui/pages/package_manager_page.h"

#include "ui/common/widget_helpers.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCheckBox>
#include <QClipboard>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QStackedWidget>
#include <QToolButton>

#include <iterator>
#include <utility>

namespace {

QLabel *makeLabel(const QString &value,
                  int size,
                  QFont::Weight weight,
                  const QString &color)
{
    auto *result = new QLabel(value);
    result->setFont(ui::appFont(size, weight));
    result->setStyleSheet(QStringLiteral("color:%1;").arg(color));
    return result;
}

QLabel *makeDetailValue()
{
    auto *value = makeLabel(ui::text("未选择软件包"), 9, QFont::Normal, "#596579");
    value->setWordWrap(true);
    value->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return value;
}

QWidget *makeDetailRow(const QString &title, QLabel *value)
{
    auto *row = new QWidget;
    auto *layout = new QVBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    layout->addWidget(makeLabel(title, 8, QFont::DemiBold, "#7b8798"));
    layout->addWidget(value);
    return row;
}

struct CatalogEntry {
    int category;
    const char *iconFile;
    const char *title;
    const char *command;
};

const CatalogEntry kCatalogEntries[] = {
    {0, "已知权限组.png", "已知权限组", "adb shell pm list permission-groups"},
    {1, "已知权限组.png", "已知权限", "adb shell pm list permissions"},
    {2, "软件包.png", "软件包", "adb shell pm list packages"},
    {3, "系统功能.png", "系统功能", "adb shell pm list features"},
    {4, "库.png", "库", "adb shell pm list libraries"},
    {5, "用户.png", "用户", "adb shell pm list users"},
};

constexpr int kInstallWorkspace = -1;

} // namespace

PackageManagerPage::PackageManagerPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("PackageManagerPage");

    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(24, 20, 24, 18);
    pageLayout->setSpacing(14);

    auto *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(10);
    titleRow->addWidget(makeLabel(ui::text("软件包管理器"), 16, QFont::DemiBold, "#111827"));
    titleRow->addWidget(makeLabel(ui::text("管理已连接 Android 设备上的应用包"),
                                  9,
                                  QFont::Normal,
                                  "#7b8798"));
    titleRow->addStretch();
    m_deviceDot = makeLabel(ui::text("●"), 12, QFont::DemiBold, "#aab3c2");
    m_deviceStatus = makeLabel(ui::text("未连接设备"), 9, QFont::DemiBold, "#596579");
    titleRow->addWidget(m_deviceDot);
    titleRow->addWidget(m_deviceStatus);
    pageLayout->addLayout(titleRow);

    auto *toolbar = ui::makePanel("PackageToolbar");
    m_packageToolbar = toolbar;
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(14, 10, 14, 10);
    toolbarLayout->setSpacing(12);

    m_searchInput = new QLineEdit;
    m_searchInput->setObjectName("PackageSearchInput");
    m_searchInput->setPlaceholderText(ui::text("搜索软件包名称"));
    m_searchInput->setClearButtonEnabled(true);
    m_searchInput->setMinimumWidth(240);
    m_searchInput->setMaximumWidth(360);
    toolbarLayout->addWidget(m_searchInput, 1);

    m_enabledFilter = new QCheckBox(ui::text("仅启用"));
    m_disabledFilter = new QCheckBox(ui::text("仅停用"));
    m_thirdPartyFilter = new QCheckBox(ui::text("第三方"));
    m_systemFilter = new QCheckBox(ui::text("系统"));
    for (QCheckBox *filter : {m_enabledFilter, m_disabledFilter, m_thirdPartyFilter, m_systemFilter}) {
        filter->setObjectName("PackageFilter");
        filter->setCursor(Qt::PointingHandCursor);
        filter->setFont(ui::appFont(9, QFont::DemiBold));
        toolbarLayout->addWidget(filter);
        connect(filter, &QCheckBox::toggled, this, &PackageManagerPage::refreshPackages);
    }

    m_refreshButton = new QToolButton;
    m_refreshButton->setObjectName("PackageRefreshButton");
    m_refreshButton->setText(ui::text("↻"));
    m_refreshButton->setToolTip(ui::text("刷新软件包列表"));
    m_refreshButton->setCursor(Qt::PointingHandCursor);
    m_refreshButton->setFixedSize(34, 34);
    m_refreshButton->setFont(ui::appFont(16, QFont::DemiBold));
    toolbarLayout->addWidget(m_refreshButton);
    connect(m_refreshButton, &QToolButton::clicked, this, &PackageManagerPage::refreshPackages);
    pageLayout->addWidget(toolbar);

    m_commandStatus = makeLabel(ui::text("连接设备后可读取软件包列表"),
                                 9,
                                 QFont::Normal,
                                 "#7b8798");
    m_commandStatus->setObjectName("PackageCommandStatus");
    m_commandStatus->setMinimumHeight(24);
    pageLayout->addWidget(m_commandStatus);

    auto *content = new QHBoxLayout;
    content->setContentsMargins(0, 0, 0, 0);
    content->setSpacing(14);

    auto *listPanel = ui::makePanel("PackageListPanel");
    listPanel->setMinimumWidth(360);
    auto *listLayout = new QVBoxLayout(listPanel);
    listLayout->setContentsMargins(16, 14, 16, 14);
    listLayout->setSpacing(10);
    auto *listTitle = new QHBoxLayout;
    listTitle->addWidget(makeLabel(ui::text("软件包列表"), 11, QFont::DemiBold, "#172033"));
    listTitle->addStretch();
    m_packageCount = makeLabel(ui::text("0 个"), 9, QFont::DemiBold, "#7b8798");
    listTitle->addWidget(m_packageCount);
    listLayout->addLayout(listTitle);

    m_packageList = new QListWidget;
    m_packageList->setObjectName("PackageList");
    m_packageList->setAlternatingRowColors(true);
    m_packageList->setSelectionMode(QAbstractItemView::SingleSelection);
    listLayout->addWidget(m_packageList, 1);
    content->addWidget(listPanel, 5);

    auto *detailPanel = ui::makePanel("PackageDetailPanel");
    detailPanel->setMinimumWidth(360);
    auto *detailLayout = new QVBoxLayout(detailPanel);
    detailLayout->setContentsMargins(18, 16, 18, 16);
    detailLayout->setSpacing(14);
    detailLayout->addWidget(makeLabel(ui::text("软件包详情"), 11, QFont::DemiBold, "#172033"));

    m_packageName = makeLabel(ui::text("未选择软件包"), 13, QFont::DemiBold, "#293243");
    m_packageName->setWordWrap(true);
    detailLayout->addWidget(m_packageName);
    m_packagePath = makeDetailValue();
    m_packageInstaller = makeDetailValue();
    detailLayout->addWidget(makeDetailRow(ui::text("APK 路径"), m_packagePath));
    detailLayout->addWidget(makeDetailRow(ui::text("安装来源"), m_packageInstaller));
    detailLayout->addStretch();

    auto *actionsTitle = makeLabel(ui::text("操作"), 9, QFont::DemiBold, "#7b8798");
    detailLayout->addWidget(actionsTitle);
    auto *actions = new QHBoxLayout;
    actions->setContentsMargins(0, 0, 0, 0);
    actions->setSpacing(8);
    m_enableButton = new QPushButton(ui::text("启用"));
    m_disableButton = new QPushButton(ui::text("停用"));
    m_clearDataButton = new QPushButton(ui::text("清数据"));
    m_uninstallButton = new QPushButton(ui::text("卸载"));
    for (QPushButton *button : {m_enableButton,
                                m_disableButton,
                                m_clearDataButton,
                                m_uninstallButton}) {
        button->setObjectName("PackageActionButton");
        button->setCursor(Qt::PointingHandCursor);
        button->setFont(ui::appFont(9, QFont::DemiBold));
        button->setMinimumHeight(34);
        actions->addWidget(button);
    }
    m_clearDataButton->setProperty("danger", true);
    m_uninstallButton->setProperty("danger", true);
    detailLayout->addLayout(actions);
    content->addWidget(detailPanel, 6);

    auto *overviewScroll = new QScrollArea;
    overviewScroll->setObjectName("PackageCatalogScroll");
    overviewScroll->setWidgetResizable(true);
    overviewScroll->setFrameShape(QFrame::NoFrame);
    auto *overviewContent = new QWidget;
    auto *overviewLayout = new QVBoxLayout(overviewContent);
    overviewLayout->setContentsMargins(0, 0, 4, 0);
    overviewLayout->setSpacing(8);

    const auto addCatalogCard = [this, overviewLayout](int category,
                                                       const QString &iconPath,
                                                       const QString &title,
                                                       const QString &command) {
        auto *card = ui::makePanel("PackageCategoryCard");
        card->setMinimumHeight(92);
        auto *cardLayout = new QHBoxLayout(card);
        cardLayout->setContentsMargins(8, 8, 12, 8);
        cardLayout->setSpacing(14);

        auto *iconPanel = ui::makePanel("PackageCategoryIcon");
        iconPanel->setFixedSize(74, 74);
        auto *iconLayout = new QVBoxLayout(iconPanel);
        iconLayout->setContentsMargins(0, 0, 0, 0);
        auto *icon = new QLabel;
        icon->setAlignment(Qt::AlignCenter);
        icon->setPixmap(ui::imagePixmap(iconPath, QSize(46, 46)));
        iconLayout->addWidget(icon);
        cardLayout->addWidget(iconPanel);

        auto *openButton = new QPushButton;
        openButton->setObjectName("PackageCategoryOpenButton");
        openButton->setCursor(Qt::PointingHandCursor);
        openButton->setFont(ui::appFont(10, QFont::Normal));
        openButton->setText(title + QLatin1Char('\n') + command);
        openButton->setToolTip(ui::text("打开 %1").arg(title));
        openButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        cardLayout->addWidget(openButton, 1);

        auto *copyButton = new QToolButton;
        copyButton->setObjectName("PackageCopyButton");
        copyButton->setText(ui::text("▣"));
        copyButton->setToolTip(ui::text("复制 ADB 命令"));
        copyButton->setCursor(Qt::PointingHandCursor);
        copyButton->setFixedSize(34, 34);
        copyButton->setFont(ui::appFont(15, QFont::Normal));
        cardLayout->addWidget(copyButton);

        auto *arrow = makeLabel(ui::text("○"), 16, QFont::Normal, "#d0d5dc");
        arrow->setFixedWidth(24);
        arrow->setAlignment(Qt::AlignCenter);
        cardLayout->addWidget(arrow);

        connect(copyButton, &QToolButton::clicked, this, [this, command] {
            copyCommand(command);
        });
        connect(openButton, &QPushButton::clicked, this, [this, category] {
            if (category == kInstallWorkspace) {
                showInstallWorkspace();
            } else if (category == 2) {
                showPackageWorkspace();
            } else {
                showCatalogWorkspace(category);
            }
        });
        overviewLayout->addWidget(card);
    };

    addCatalogCard(kInstallWorkspace,
                   QStringLiteral("icons/软件包管理器/安装应用.png"),
                   ui::text("安装应用"),
                   ui::text("adb install <filename>"));
    for (const CatalogEntry &entry : kCatalogEntries) {
        addCatalogCard(entry.category,
                       QStringLiteral("icons/软件包管理器/%1")
                           .arg(QString::fromUtf8(entry.iconFile)),
                       ui::text(entry.title),
                       ui::text(entry.command));
    }
    overviewLayout->addStretch();
    overviewScroll->setWidget(overviewContent);

    auto *packageWorkspace = new QWidget;
    packageWorkspace->setLayout(content);

    auto *resultWorkspace = new QWidget;
    auto *resultLayout = new QVBoxLayout(resultWorkspace);
    resultLayout->setContentsMargins(8, 0, 8, 0);
    resultLayout->setSpacing(12);
    auto *resultHeader = new QHBoxLayout;
    resultHeader->setContentsMargins(0, 0, 0, 0);
    resultHeader->setSpacing(10);
    m_resultBackButton = new QToolButton;
    m_resultBackButton->setObjectName("PackageBackButton");
    m_resultBackButton->setText(ui::text("←"));
    m_resultBackButton->setToolTip(ui::text("返回软件包管理器"));
    m_resultBackButton->setCursor(Qt::PointingHandCursor);
    m_resultBackButton->setFixedSize(34, 34);
    m_resultBackButton->setFont(ui::appFont(16, QFont::DemiBold));
    resultHeader->addWidget(m_resultBackButton);
    m_resultTitle = makeLabel(ui::text("查询结果"), 13, QFont::DemiBold, "#172033");
    resultHeader->addWidget(m_resultTitle);
    resultHeader->addStretch();
    m_resultStatus = makeLabel(ui::text("未查询"), 9, QFont::Normal, "#7b8798");
    resultHeader->addWidget(m_resultStatus);
    m_resultRemoveButton = new QPushButton(ui::text("删除用户"));
    m_resultRemoveButton->setObjectName("PackageResultActionButton");
    m_resultRemoveButton->setCursor(Qt::PointingHandCursor);
    m_resultRemoveButton->setFont(ui::appFont(9, QFont::DemiBold));
    m_resultRemoveButton->setMinimumHeight(32);
    m_resultRemoveButton->setVisible(false);
    resultHeader->addWidget(m_resultRemoveButton);
    resultLayout->addLayout(resultHeader);

    m_resultList = new QListWidget;
    m_resultList->setObjectName("PackageResultList");
    m_resultList->setAlternatingRowColors(true);
    resultLayout->addWidget(m_resultList, 1);

    auto *installWorkspace = new QWidget;
    auto *installLayout = new QVBoxLayout(installWorkspace);
    installLayout->setContentsMargins(8, 0, 8, 0);
    installLayout->setSpacing(12);

    auto *installHeader = new QHBoxLayout;
    installHeader->setContentsMargins(0, 0, 0, 0);
    installHeader->setSpacing(10);
    m_installBackButton = new QToolButton;
    m_installBackButton->setObjectName("PackageBackButton");
    m_installBackButton->setText(ui::text("←"));
    m_installBackButton->setToolTip(ui::text("返回软件包管理器"));
    m_installBackButton->setCursor(Qt::PointingHandCursor);
    m_installBackButton->setFixedSize(34, 34);
    m_installBackButton->setFont(ui::appFont(16, QFont::DemiBold));
    installHeader->addWidget(m_installBackButton);
    installHeader->addWidget(makeLabel(ui::text("安装应用"),
                                        13,
                                        QFont::DemiBold,
                                        "#172033"));
    installHeader->addStretch();
    installLayout->addLayout(installHeader);

    auto *filePanel = ui::makePanel("PackageInstallPanel");
    auto *fileLayout = new QHBoxLayout(filePanel);
    fileLayout->setContentsMargins(14, 12, 14, 12);
    fileLayout->setSpacing(10);
    m_installSelectButton = new QToolButton;
    m_installSelectButton->setObjectName("PackageInstallSelectButton");
    m_installSelectButton->setText(ui::text("选择 APK"));
    m_installSelectButton->setToolTip(ui::text("选择 APK 文件"));
    m_installSelectButton->setCursor(Qt::PointingHandCursor);
    m_installSelectButton->setFixedSize(88, 34);
    m_installSelectButton->setFont(ui::appFont(9, QFont::DemiBold));
    fileLayout->addWidget(m_installSelectButton);
    m_installFilePath = new QLineEdit;
    m_installFilePath->setObjectName("PackageInstallFilePath");
    m_installFilePath->setPlaceholderText(ui::text("选择要安装的 APK 文件"));
    m_installFilePath->setReadOnly(true);
    fileLayout->addWidget(m_installFilePath, 1);
    installLayout->addWidget(filePanel);

    auto *noticePanel = ui::makePanel("PackageInstallNotice");
    auto *noticeLayout = new QHBoxLayout(noticePanel);
    noticeLayout->setContentsMargins(14, 12, 14, 12);
    noticeLayout->setSpacing(10);
    noticeLayout->addWidget(makeLabel(ui::text("!"), 14, QFont::DemiBold, "#2f7d62"));
    auto *notice = makeLabel(ui::text("安装过程中请保持设备连接，不要插拔设备。"),
                             9,
                             QFont::DemiBold,
                             "#2f6b57");
    notice->setWordWrap(true);
    noticeLayout->addWidget(notice, 1);
    installLayout->addWidget(noticePanel);

    auto *installActions = new QHBoxLayout;
    installActions->setContentsMargins(0, 0, 0, 0);
    installActions->setSpacing(10);
    m_installButton = new QPushButton(ui::text("▶  安装应用"));
    m_installButton->setObjectName("PackageInstallStartButton");
    m_installButton->setCursor(Qt::PointingHandCursor);
    m_installButton->setFont(ui::appFont(9, QFont::DemiBold));
    m_installButton->setMinimumSize(124, 36);
    installActions->addWidget(m_installButton);
    installActions->addStretch();
    installLayout->addLayout(installActions);

    m_installProgress = new QProgressBar;
    m_installProgress->setObjectName("PackageInstallProgress");
    m_installProgress->setRange(0, 0);
    m_installProgress->setTextVisible(false);
    m_installProgress->setVisible(false);
    installLayout->addWidget(m_installProgress);
    installLayout->addStretch();

    m_workspaceStack = new QStackedWidget;
    m_workspaceStack->setObjectName("PackageWorkspaceStack");
    m_workspaceStack->addWidget(overviewScroll);
    m_workspaceStack->addWidget(packageWorkspace);
    m_workspaceStack->addWidget(resultWorkspace);
    m_workspaceStack->addWidget(installWorkspace);
    pageLayout->addWidget(m_workspaceStack, 1);
    m_packageToolbar->setVisible(false);

    connect(m_resultBackButton, &QToolButton::clicked, this, &PackageManagerPage::showOverview);
    connect(m_installBackButton, &QToolButton::clicked, this, &PackageManagerPage::showOverview);
    connect(m_installSelectButton, &QToolButton::clicked, this, &PackageManagerPage::selectApkFile);
    connect(m_installButton, &QPushButton::clicked, this, &PackageManagerPage::startInstall);
    connect(m_resultRemoveButton, &QPushButton::clicked, this, [this] {
        const QListWidgetItem *item = m_resultList->currentItem();
        if (item == nullptr) {
            QMessageBox::information(this, ui::text("删除用户"), ui::text("请先选择一个用户。"));
            return;
        }
        static const QRegularExpression userIdPattern(QStringLiteral("UserInfo\\{(\\d+):"));
        const QRegularExpressionMatch match = userIdPattern.match(item->text());
        if (!match.hasMatch()) {
            QMessageBox::information(this,
                                     ui::text("删除用户"),
                                     ui::text("所选条目不包含有效的 Android 用户 ID。"));
            return;
        }
        const QString userId = match.captured(1);
        if (userId == QStringLiteral("0")) {
            QMessageBox::warning(this,
                                 ui::text("无法删除"),
                                 ui::text("Android 主用户不能通过此处删除。"));
            return;
        }
        const QMessageBox::StandardButton choice = QMessageBox::warning(
            this,
            ui::text("确认删除用户"),
            ui::text("确定要删除 Android 用户 %1 吗？该用户的数据也会被删除。").arg(userId),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (choice == QMessageBox::Yes) {
            emit userRemoveRequested(userId);
        }
    });

    connect(m_searchInput, &QLineEdit::textChanged, this, &PackageManagerPage::applySearch);
    connect(m_packageList,
            &QListWidget::currentItemChanged,
            this,
            [this](QListWidgetItem *current) {
                if (current == nullptr) {
                    clearDetails();
                    return;
                }
                m_selectedPackage = current->text();
                m_packageName->setText(m_selectedPackage);
                m_packagePath->setText(ui::text("正在读取 APK 路径…"));
                m_packageInstaller->setText(ui::text("正在读取安装来源…"));
                emit packageDetailsRequested(m_selectedPackage);
                updateControls();
            });
    connect(m_enableButton, &QPushButton::clicked, this, [this] {
        requestAction(ui::text("确认启用"),
                      ui::text("确定要启用软件包“%1”吗？").arg(m_selectedPackage),
                      &PackageManagerPage::enableRequested);
    });
    connect(m_disableButton, &QPushButton::clicked, this, [this] {
        requestAction(ui::text("确认停用"),
                      ui::text("确定要停用软件包“%1”吗？应用将无法运行，直到再次启用。").arg(
                          m_selectedPackage),
                      &PackageManagerPage::disableRequested);
    });
    connect(m_clearDataButton, &QPushButton::clicked, this, [this] {
        requestAction(ui::text("确认清除应用数据"),
                      ui::text("确定要清除“%1”的全部应用数据吗？此操作不可撤销。").arg(
                          m_selectedPackage),
                      &PackageManagerPage::clearDataRequested);
    });
    connect(m_uninstallButton, &QPushButton::clicked, this, [this] {
        requestAction(ui::text("确认卸载软件包"),
                      ui::text("确定要卸载“%1”吗？此操作不可撤销。").arg(m_selectedPackage),
                      &PackageManagerPage::uninstallRequested);
    });

    updateControls();
}

void PackageManagerPage::setDeviceConnected(bool connected, const QString &serial)
{
    if (m_installRunning && (!connected || serial != m_serial)) {
        m_installRunning = false;
        m_installProgress->setVisible(false);
    }
    m_connected = connected;
    m_serial = connected ? serial : QString();
    m_deviceDot->setStyleSheet(connected ? QStringLiteral("color:#66c95e;")
                                          : QStringLiteral("color:#aab3c2;"));
    m_deviceStatus->setText(connected
                                ? ui::text("已连接 · %1").arg(serial)
                                : ui::text("未连接设备"));
    if (connected && !m_busy) {
        m_commandStatus->setStyleSheet(QStringLiteral("color:#7b8798;"));
        m_commandStatus->setText(ui::text("选择一项以查询设备信息"));
    }
    if (!connected) {
        m_packages.clear();
        applySearch();
        clearDetails();
        showOverview();
        m_commandStatus->setStyleSheet(QStringLiteral("color:#7b8798;"));
        m_commandStatus->setText(ui::text("连接设备后可读取软件包列表"));
    }
    updateControls();
}

void PackageManagerPage::setPackages(const QStringList &packages)
{
    m_packages = packages;
    applySearch();
}

void PackageManagerPage::setPackageDetails(const QString &packageName,
                                           const QString &path,
                                           const QString &installer)
{
    if (packageName != m_selectedPackage) {
        return;
    }
    m_packageName->setText(packageName);
    m_packagePath->setText(path);
    m_packageInstaller->setText(installer);
}

void PackageManagerPage::setCategoryResults(int category, const QStringList &items)
{
    if (category < 0 || category >= static_cast<int>(std::size(kCatalogEntries)) || category == 2) {
        return;
    }

    m_resultTitle->setText(ui::text(kCatalogEntries[category].title));
    m_resultCategory = category;
    m_resultRemoveButton->setVisible(category == 5);
    m_resultRemoveButton->setEnabled(category == 5 && !m_busy);
    m_resultList->clear();
    m_resultList->addItems(items);
    m_resultStatus->setText(ui::text("%1 项").arg(items.size()));
}

void PackageManagerPage::setBusy(bool busy)
{
    m_busy = busy;
    updateControls();
    if (m_resultBackButton != nullptr) {
        m_resultBackButton->setEnabled(!busy);
    }
    if (m_resultList != nullptr) {
        m_resultList->setEnabled(!busy);
    }
    if (m_resultRemoveButton != nullptr) {
        m_resultRemoveButton->setEnabled(!busy && m_resultCategory == 5);
    }
    if (m_installProgress != nullptr) {
        m_installProgress->setVisible(m_installRunning && busy);
    }
}

void PackageManagerPage::showCommandStarted(const QString &label, const QString &command)
{
    m_commandStatus->setStyleSheet(QStringLiteral("color:#2f6df6;"));
    m_commandStatus->setText(ui::text("正在执行：%1").arg(label));
    m_commandStatus->setToolTip(command);
}

void PackageManagerPage::showCommandResult(bool success,
                                           const QString &label,
                                           const QString &detail)
{
    m_commandStatus->setStyleSheet(success ? QStringLiteral("color:#459b47;")
                                            : QStringLiteral("color:#d45b5b;"));
    m_commandStatus->setText(success
                                 ? ui::text("%1：%2").arg(label, detail)
                                 : ui::text("%1失败：%2").arg(label, detail));
    if (m_installRunning) {
        m_installRunning = false;
        m_installProgress->setVisible(false);
        updateControls();
    }
}

void PackageManagerPage::refreshPackages()
{
    if (!m_connected || m_busy) {
        return;
    }
    emit packageListRefreshRequested(m_enabledFilter->isChecked(),
                                     m_disabledFilter->isChecked(),
                                     m_thirdPartyFilter->isChecked(),
                                     m_systemFilter->isChecked());
}

void PackageManagerPage::showOverview()
{
    if (m_workspaceStack == nullptr || m_busy) {
        return;
    }
    m_workspaceStack->setCurrentIndex(0);
    m_packageToolbar->setVisible(false);
    m_resultCategory = -1;
    m_resultRemoveButton->setVisible(false);
}

void PackageManagerPage::showPackageWorkspace()
{
    if (m_workspaceStack == nullptr) {
        return;
    }
    m_workspaceStack->setCurrentIndex(1);
    m_packageToolbar->setVisible(true);
    m_resultCategory = -1;
    m_resultRemoveButton->setVisible(false);
    if (m_connected) {
        refreshPackages();
    }
}

void PackageManagerPage::showInstallWorkspace()
{
    if (m_workspaceStack == nullptr || m_busy) {
        return;
    }
    m_workspaceStack->setCurrentIndex(3);
    m_packageToolbar->setVisible(false);
    m_resultCategory = -1;
    m_resultRemoveButton->setVisible(false);
    updateControls();
}

void PackageManagerPage::showCatalogWorkspace(int category)
{
    if (m_workspaceStack == nullptr || category < 0 || category >= 6 || category == 2) {
        return;
    }
    m_workspaceStack->setCurrentIndex(2);
    m_packageToolbar->setVisible(false);
    m_resultCategory = category;
    m_resultRemoveButton->setVisible(category == 5);
    m_resultTitle->setText(ui::text(kCatalogEntries[category].title));
    m_resultList->clear();
    m_resultStatus->setText(m_connected ? ui::text("正在查询") : ui::text("未连接设备"));
    if (m_connected) {
        emit categoryRequested(category);
    }
}

void PackageManagerPage::selectApkFile()
{
    const QString selected = QFileDialog::getOpenFileName(this,
                                                          ui::text("选择 APK 安装包"),
                                                          QString(),
                                                          ui::text("APK 文件 (*.apk)"));
    if (!selected.isEmpty()) {
        m_installFilePath->setText(QDir::toNativeSeparators(selected));
    }
    updateControls();
}

void PackageManagerPage::startInstall()
{
    const QFileInfo apkFile(m_installFilePath->text());
    if (!apkFile.isFile()
        || apkFile.suffix().compare(QStringLiteral("apk"), Qt::CaseInsensitive) != 0) {
        QMessageBox::warning(this,
                             ui::text("未选择安装包"),
                             ui::text("请选择有效的 .apk 文件。"));
        return;
    }
    if (!m_connected) {
        QMessageBox::warning(this,
                             ui::text("无法安装应用"),
                             ui::text("请先连接并授权 Android 设备。"));
        return;
    }

    m_installRunning = true;
    m_installProgress->setRange(0, 0);
    m_installProgress->setVisible(true);
    updateControls();
    emit installRequested(apkFile.absoluteFilePath());
}

void PackageManagerPage::copyCommand(const QString &command)
{
    QApplication::clipboard()->setText(command);
    m_commandStatus->setStyleSheet(QStringLiteral("color:#459b47;"));
    m_commandStatus->setText(ui::text("已复制 ADB 命令"));
    m_commandStatus->setToolTip(command);
}

void PackageManagerPage::applySearch()
{
    const QString previousSelection = m_selectedPackage;
    const QString query = m_searchInput == nullptr ? QString() : m_searchInput->text().trimmed();
    m_packageList->clear();
    for (const QString &packageName : std::as_const(m_packages)) {
        if (!query.isEmpty() && !packageName.contains(query, Qt::CaseInsensitive)) {
            continue;
        }
        m_packageList->addItem(packageName);
    }

    m_packageCount->setText(ui::text("%1 个").arg(m_packageList->count()));
    const QList<QListWidgetItem *> matches = m_packageList->findItems(previousSelection, Qt::MatchExactly);
    if (!matches.isEmpty()) {
        m_packageList->setCurrentItem(matches.first());
    } else {
        clearDetails();
    }
}

void PackageManagerPage::clearDetails()
{
    m_selectedPackage.clear();
    m_packageName->setText(ui::text("未选择软件包"));
    m_packagePath->setText(ui::text("选择软件包后显示 APK 路径"));
    m_packageInstaller->setText(ui::text("选择软件包后显示安装来源"));
    updateControls();
}

void PackageManagerPage::updateControls()
{
    const bool canRefresh = m_connected && !m_busy;
    const bool canAct = canRefresh && !m_selectedPackage.isEmpty();
    m_searchInput->setEnabled(m_connected && !m_busy);
    m_packageList->setEnabled(m_connected && !m_busy);
    m_enabledFilter->setEnabled(canRefresh);
    m_disabledFilter->setEnabled(canRefresh);
    m_thirdPartyFilter->setEnabled(canRefresh);
    m_systemFilter->setEnabled(canRefresh);
    m_refreshButton->setEnabled(canRefresh);
    for (QPushButton *button : {m_uninstallButton,
                                m_clearDataButton,
                                m_enableButton,
                                m_disableButton}) {
        button->setEnabled(canAct);
    }
    const QFileInfo apkFile(m_installFilePath->text());
    const bool hasApk = apkFile.isFile()
        && apkFile.suffix().compare(QStringLiteral("apk"), Qt::CaseInsensitive) == 0;
    const bool installIdle = !m_busy && !m_installRunning;
    m_installBackButton->setEnabled(installIdle);
    m_installSelectButton->setEnabled(installIdle);
    m_installButton->setEnabled(installIdle && m_connected && hasApk);
}

void PackageManagerPage::requestAction(const QString &title,
                                       const QString &message,
                                       void (PackageManagerPage::*signal)(const QString &))
{
    if (m_selectedPackage.isEmpty() || m_busy) {
        return;
    }
    const QMessageBox::StandardButton choice = QMessageBox::warning(this,
                                                                      title,
                                                                      message,
                                                                      QMessageBox::Yes | QMessageBox::No,
                                                                      QMessageBox::No);
    if (choice == QMessageBox::Yes) {
        (this->*signal)(m_selectedPackage);
    }
}
