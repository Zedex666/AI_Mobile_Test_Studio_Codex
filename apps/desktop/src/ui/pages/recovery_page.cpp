#include "ui/pages/recovery_page.h"

#include "ui/common/widget_helpers.h"

#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QStackedWidget>
#include <QToolButton>

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

} // namespace

RecoveryPage::RecoveryPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("RecoveryPage");
    auto *pageLayout = new QVBoxLayout(this);
    pageLayout->setContentsMargins(24, 20, 24, 18);
    pageLayout->setSpacing(14);

    auto *titleRow = new QHBoxLayout;
    titleRow->setContentsMargins(0, 0, 0, 0);
    titleRow->setSpacing(10);
    titleRow->addWidget(makeLabel(ui::text("Recovery"), 16, QFont::DemiBold, "#111827"));
    titleRow->addStretch();
    m_deviceDot = makeLabel(ui::text("●"), 12, QFont::DemiBold, "#aab3c2");
    m_deviceStatus = makeLabel(ui::text("未连接设备"), 9, QFont::DemiBold, "#596579");
    titleRow->addWidget(m_deviceDot);
    titleRow->addWidget(m_deviceStatus);
    pageLayout->addLayout(titleRow);

    m_operationStatus = makeLabel(ui::text("进入侧载功能后选择 ZIP 文件"),
                                  9,
                                  QFont::Normal,
                                  "#7b8798");
    m_operationStatus->setMinimumHeight(24);
    pageLayout->addWidget(m_operationStatus);

    auto *overview = new QWidget;
    auto *overviewLayout = new QVBoxLayout(overview);
    overviewLayout->setContentsMargins(0, 4, 0, 0);
    overviewLayout->setSpacing(8);

    auto *entry = ui::makePanel("RecoveryEntryCard");
    entry->setMinimumHeight(96);
    entry->setMaximumHeight(112);
    auto *entryLayout = new QHBoxLayout(entry);
    entryLayout->setContentsMargins(8, 8, 12, 8);
    entryLayout->setSpacing(14);

    auto *iconPanel = ui::makePanel("RecoveryEntryIcon");
    iconPanel->setFixedSize(74, 74);
    auto *iconLayout = new QVBoxLayout(iconPanel);
    iconLayout->setContentsMargins(0, 0, 0, 0);
    auto *icon = makeLabel(ui::text("▰"), 28, QFont::Normal, "#172033");
    icon->setAlignment(Qt::AlignCenter);
    iconLayout->addWidget(icon);
    entryLayout->addWidget(iconPanel);

    auto *openButton = new QPushButton(ui::text("侧载\nadb sideload <filename>"));
    openButton->setObjectName("RecoveryEntryOpenButton");
    openButton->setToolTip(ui::text("打开 Recovery 侧载"));
    openButton->setCursor(Qt::PointingHandCursor);
    openButton->setFont(ui::appFont(10, QFont::Normal));
    openButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    entryLayout->addWidget(openButton, 1);

    auto *copyButton = new QToolButton;
    copyButton->setObjectName("RecoveryCopyButton");
    copyButton->setText(ui::text("▣"));
    copyButton->setToolTip(ui::text("复制 ADB 命令"));
    copyButton->setCursor(Qt::PointingHandCursor);
    copyButton->setFixedSize(34, 34);
    copyButton->setFont(ui::appFont(15, QFont::Normal));
    entryLayout->addWidget(copyButton);

    auto *arrow = makeLabel(ui::text("○"), 16, QFont::Normal, "#d0d5dc");
    arrow->setFixedWidth(24);
    arrow->setAlignment(Qt::AlignCenter);
    entryLayout->addWidget(arrow);
    overviewLayout->addWidget(entry);
    overviewLayout->addStretch();

    auto *sideloadWorkspace = new QWidget;
    auto *sideloadLayout = new QVBoxLayout(sideloadWorkspace);
    sideloadLayout->setContentsMargins(0, 0, 0, 0);
    sideloadLayout->setSpacing(12);

    auto *workspaceHeader = new QHBoxLayout;
    workspaceHeader->setContentsMargins(0, 0, 0, 0);
    workspaceHeader->setSpacing(10);
    m_backButton = new QToolButton;
    m_backButton->setObjectName("RecoveryBackButton");
    m_backButton->setText(ui::text("←"));
    m_backButton->setToolTip(ui::text("返回 Recovery"));
    m_backButton->setCursor(Qt::PointingHandCursor);
    m_backButton->setFixedSize(34, 34);
    m_backButton->setFont(ui::appFont(16, QFont::DemiBold));
    workspaceHeader->addWidget(m_backButton);
    workspaceHeader->addWidget(makeLabel(ui::text("ADB Sideload"),
                                          13,
                                          QFont::DemiBold,
                                          "#172033"));
    workspaceHeader->addStretch();
    sideloadLayout->addLayout(workspaceHeader);

    auto *modePanel = ui::makePanel("RecoveryModePanel");
    auto *modeLayout = new QHBoxLayout(modePanel);
    modeLayout->setContentsMargins(14, 12, 14, 12);
    modeLayout->setSpacing(10);
    modeLayout->addWidget(makeLabel(ui::text("!"), 14, QFont::DemiBold, "#b7791f"));
    m_modeMessage = makeLabel(ui::text("请先在设备 Recovery 中启用 ADB Sideload"),
                              9,
                              QFont::DemiBold,
                              "#765618");
    m_modeMessage->setWordWrap(true);
    modeLayout->addWidget(m_modeMessage, 1);
    sideloadLayout->addWidget(modePanel);

    auto *filePanel = ui::makePanel("RecoveryFilePanel");
    auto *fileLayout = new QHBoxLayout(filePanel);
    fileLayout->setContentsMargins(14, 12, 14, 12);
    fileLayout->setSpacing(10);
    m_filePath = new QLineEdit;
    m_filePath->setObjectName("RecoveryFilePath");
    m_filePath->setPlaceholderText(ui::text("选择用于侧载的 ZIP 文件"));
    m_filePath->setReadOnly(true);
    fileLayout->addWidget(m_filePath, 1);
    m_selectButton = new QToolButton;
    m_selectButton->setObjectName("RecoverySelectButton");
    m_selectButton->setText(ui::text("…"));
    m_selectButton->setToolTip(ui::text("选择 ZIP 文件"));
    m_selectButton->setCursor(Qt::PointingHandCursor);
    m_selectButton->setFixedSize(38, 34);
    m_selectButton->setFont(ui::appFont(14, QFont::DemiBold));
    fileLayout->addWidget(m_selectButton);
    sideloadLayout->addWidget(filePanel);

    auto *actions = new QHBoxLayout;
    actions->setContentsMargins(0, 0, 0, 0);
    actions->setSpacing(10);
    m_startButton = new QPushButton(ui::text("开始侧载"));
    m_startButton->setObjectName("RecoveryStartButton");
    m_startButton->setCursor(Qt::PointingHandCursor);
    m_startButton->setFont(ui::appFont(9, QFont::DemiBold));
    m_startButton->setMinimumSize(116, 36);
    actions->addWidget(m_startButton);
    m_cancelButton = new QPushButton(ui::text("取消"));
    m_cancelButton->setObjectName("RecoveryCancelButton");
    m_cancelButton->setCursor(Qt::PointingHandCursor);
    m_cancelButton->setFont(ui::appFont(9, QFont::DemiBold));
    m_cancelButton->setMinimumSize(88, 36);
    m_cancelButton->setVisible(false);
    actions->addWidget(m_cancelButton);
    actions->addStretch();
    sideloadLayout->addLayout(actions);

    m_progressBar = new QProgressBar;
    m_progressBar->setObjectName("RecoveryProgressBar");
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setVisible(false);
    sideloadLayout->addWidget(m_progressBar);

    m_output = new QPlainTextEdit;
    m_output->setObjectName("RecoveryOutput");
    m_output->setReadOnly(true);
    m_output->setPlainText(ui::text("暂时仅支持 .zip 格式\n侧载进度也会显示在设备端"));
    sideloadLayout->addWidget(m_output, 1);

    m_workspaceStack = new QStackedWidget;
    m_workspaceStack->setObjectName("RecoveryWorkspaceStack");
    m_workspaceStack->addWidget(overview);
    m_workspaceStack->addWidget(sideloadWorkspace);
    pageLayout->addWidget(m_workspaceStack, 1);

    connect(openButton, &QPushButton::clicked, this, &RecoveryPage::showSideloadWorkspace);
    connect(copyButton, &QToolButton::clicked, this, &RecoveryPage::copyCommand);
    connect(m_backButton, &QToolButton::clicked, this, &RecoveryPage::showOverview);
    connect(m_selectButton, &QToolButton::clicked, this, &RecoveryPage::selectZipFile);
    connect(m_startButton, &QPushButton::clicked, this, &RecoveryPage::startSideload);
    connect(m_cancelButton, &QPushButton::clicked, this, &RecoveryPage::cancelRequested);
    updateControls();
}

void RecoveryPage::setDeviceState(bool connected, bool sideloadMode, const QString &serial)
{
    m_connected = connected;
    m_sideloadMode = sideloadMode;
    m_serial = connected ? serial : QString();

    QString color = QStringLiteral("#aab3c2");
    QString status = ui::text("未连接设备");
    if (sideloadMode) {
        color = QStringLiteral("#66c95e");
        status = ui::text("Sideload · %1").arg(serial);
        m_modeMessage->setText(ui::text("设备已进入 Sideload 模式。该功能会修改设备，请谨慎使用。"));
        m_modeMessage->setStyleSheet(QStringLiteral("color:#a24b42;"));
    } else if (connected) {
        color = QStringLiteral("#e2a43a");
        status = ui::text("已连接 · 未进入 Sideload");
        m_modeMessage->setText(ui::text("请先在设备 Recovery 中选择 Apply update from ADB。"));
        m_modeMessage->setStyleSheet(QStringLiteral("color:#765618;"));
    } else {
        m_modeMessage->setText(ui::text("请连接设备并在 Recovery 中启用 ADB Sideload。"));
        m_modeMessage->setStyleSheet(QStringLiteral("color:#765618;"));
    }

    m_deviceDot->setStyleSheet(QStringLiteral("color:%1;").arg(color));
    m_deviceStatus->setText(status);
    updateControls();
}

void RecoveryPage::showOverview()
{
    if (m_busy) {
        return;
    }
    m_workspaceStack->setCurrentIndex(0);
}

void RecoveryPage::setBusy(bool busy)
{
    m_busy = busy;
    m_progressBar->setVisible(busy || m_progressBar->value() > 0);
    m_cancelButton->setVisible(busy);
    updateControls();
}

void RecoveryPage::showSideloadStarted(const QString &displayCommand)
{
    m_displayCommand = displayCommand;
    m_operationStatus->setStyleSheet(QStringLiteral("color:#2f6df6;"));
    m_operationStatus->setText(ui::text("正在侧载，请勿断开设备"));
    m_progressBar->setRange(0, 0);
    m_progressBar->setVisible(true);
    m_output->setPlainText(displayCommand + ui::text("\n\n等待 ADB 输出…"));
}

void RecoveryPage::setOutput(const QString &output)
{
    m_output->setPlainText(m_displayCommand + QStringLiteral("\n\n") + output);
    m_output->verticalScrollBar()->setValue(m_output->verticalScrollBar()->maximum());
}

void RecoveryPage::setProgress(int progress)
{
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(progress);
    m_progressBar->setFormat(ui::text("传输进度 %p%"));
}

void RecoveryPage::showSideloadFinished(bool success, const QString &detail)
{
    m_operationStatus->setStyleSheet(success ? QStringLiteral("color:#459b47;")
                                              : QStringLiteral("color:#d45b5b;"));
    m_operationStatus->setText(success
                                   ? ui::text("侧载传输已结束，请在设备端确认结果")
                                   : detail);
    if (success) {
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(100);
        m_progressBar->setVisible(true);
        m_filePath->clear();
    }
    if (m_output->toPlainText().trimmed().isEmpty()) {
        m_output->setPlainText(detail);
    }
    updateControls();
}

void RecoveryPage::showSideloadWorkspace()
{
    m_workspaceStack->setCurrentIndex(1);
}

void RecoveryPage::selectZipFile()
{
    const QString selected = QFileDialog::getOpenFileName(this,
                                                          ui::text("选择 Recovery ZIP 包"),
                                                          QString(),
                                                          ui::text("ZIP 文件 (*.zip)"));
    if (!selected.isEmpty()) {
        m_filePath->setText(selected);
    }
    updateControls();
}

void RecoveryPage::startSideload()
{
    if (!m_sideloadMode) {
        QMessageBox::warning(this,
                             ui::text("无法开始侧载"),
                             ui::text("请先在设备 Recovery 中选择 Apply update from ADB。"));
        return;
    }
    if (m_filePath->text().isEmpty() || !QFileInfo::exists(m_filePath->text())) {
        QMessageBox::warning(this,
                             ui::text("未选择文件"),
                             ui::text("请选择有效的 .zip 文件。"));
        return;
    }

    const QMessageBox::StandardButton choice = QMessageBox::warning(
        this,
        ui::text("确认开始 Recovery 侧载"),
        ui::text("侧载会把所选 ZIP 包发送到设备。请确认文件适用于当前设备，并保持连接稳定。"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (choice == QMessageBox::Yes) {
        emit sideloadRequested(m_filePath->text());
    }
}

void RecoveryPage::copyCommand()
{
    QApplication::clipboard()->setText(QStringLiteral("adb sideload <filename>"));
    m_operationStatus->setStyleSheet(QStringLiteral("color:#459b47;"));
    m_operationStatus->setText(ui::text("已复制 ADB 侧载命令"));
}

void RecoveryPage::updateControls()
{
    const bool hasZip = !m_filePath->text().isEmpty();
    m_backButton->setEnabled(!m_busy);
    m_selectButton->setEnabled(!m_busy);
    m_startButton->setEnabled(!m_busy && m_sideloadMode && hasZip);
    m_cancelButton->setEnabled(m_busy);
}
