#include "ui/pages/mirroring_page.h"

#include "ui/common/widget_helpers.h"

#include <QBoxLayout>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGridLayout>
#include <QDoubleValidator>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSize>
#include <QStandardPaths>
#include <QStyle>
#include <QToolButton>

#include <algorithm>

namespace {

QLabel *makeLabel(const QString &text,
                  int pointSize = 9,
                  QFont::Weight weight = QFont::Normal)
{
    auto *label = new QLabel(text);
    label->setFont(ui::appFont(pointSize, weight));
    return label;
}

QFrame *makeCard(const QString &iconPath, const QString &title, QVBoxLayout **contentLayout)
{
    auto *card = new QFrame;
    card->setObjectName("MirrorCard");
    card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(16, 14, 16, 16);
    layout->setSpacing(12);

    auto *header = new QHBoxLayout;
    auto *iconLabel = new QLabel;
    iconLabel->setObjectName("MirrorCardIcon");
    iconLabel->setFixedSize(34, 34);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setPixmap(ui::imagePixmap(iconPath, QSize(24, 24)));
    header->addWidget(iconLabel);
    auto *titleLabel = makeLabel(title, 10, QFont::DemiBold);
    titleLabel->setObjectName("MirrorCardTitle");
    header->addWidget(titleLabel);
    header->addStretch();
    layout->addLayout(header);

    *contentLayout = layout;
    return card;
}

QWidget *makeField(const QString &labelText, QWidget *field)
{
    auto *container = new QWidget;
    container->setObjectName("MirrorField");
    container->setMinimumWidth(0);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);
    auto *label = makeLabel(labelText, 8);
    label->setObjectName("MirrorFieldLabel");
    layout->addWidget(label);
    layout->addWidget(field);
    return container;
}

QLineEdit *makeNumberInput(const QString &placeholder)
{
    auto *input = new QLineEdit;
    input->setObjectName("MirrorInput");
    input->setPlaceholderText(placeholder);
    input->setClearButtonEnabled(true);
    input->setMinimumWidth(0);
    return input;
}

QCheckBox *makeToggle(const QString &text, bool checked = false)
{
    auto *check = new QCheckBox(text);
    check->setObjectName("MirrorToggle");
    check->setChecked(checked);
    check->setCursor(Qt::PointingHandCursor);
    check->setMinimumHeight(42);
    return check;
}

void addComboOption(QComboBox *combo,
                    const QString &label,
                    const QString &value)
{
    combo->addItem(label, value);
}

bool validInteger(const QString &value, int minimum, int maximum)
{
    bool ok = false;
    const int parsed = value.toInt(&ok);
    return ok && parsed >= minimum && parsed <= maximum;
}

bool validNumber(const QString &value, double minimum, double maximum)
{
    bool ok = false;
    const double parsed = value.toDouble(&ok);
    return ok && parsed >= minimum && parsed <= maximum;
}

} // namespace

MirroringPage::MirroringPage(QWidget *parent)
    : QWidget(parent)
{
    setObjectName("MirroringPage");
    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setObjectName("MirroringToolbar");
    toolbar->setFixedHeight(48);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(22, 0, 18, 0);
    toolbarLayout->setSpacing(12);
    auto *title = makeLabel(QStringLiteral("Mirroring"), 11, QFont::DemiBold);
    m_statusLabel = makeLabel(ui::text("等待设备连接"), 9);
    m_statusLabel->setObjectName("MirroringStatus");
    toolbarLayout->addWidget(title);
    toolbarLayout->addWidget(m_statusLabel);
    toolbarLayout->addStretch();
    rootLayout->addWidget(toolbar);

    auto *modeBar = new QWidget;
    modeBar->setObjectName("MirrorModeBar");
    modeBar->setFixedHeight(66);
    auto *modeLayout = new QHBoxLayout(modeBar);
    modeLayout->setContentsMargins(22, 6, 22, 0);
    modeLayout->setSpacing(8);
    m_modeGroup = new QButtonGroup(this);
    m_modeGroup->setExclusive(true);
    struct ModeOption {
        QString label;
        QString tooltip;
        QString iconPath;
    };
    const QVector<ModeOption> modes = {
        {ui::text("主屏幕"), ui::text("镜像设备主显示器"), QStringLiteral("icons/镜像/主屏幕.png")},
        {ui::text("虚拟屏幕"),
         ui::text("创建可调整大小的虚拟显示器"),
         QStringLiteral("icons/镜像/虚拟屏幕.png")},
        {ui::text("摄像头"),
         ui::text("镜像设备摄像头画面"),
         QStringLiteral("icons/镜像/摄像头.png")}};
    for (int index = 0; index < modes.size(); ++index) {
        auto *button = new QToolButton;
        button->setObjectName("MirrorModeButton");
        button->setText(modes[index].label);
        button->setToolTip(modes[index].tooltip);
        button->setIcon(ui::imageIcon(modes[index].iconPath));
        button->setIconSize(QSize(22, 22));
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button->setCheckable(true);
        button->setChecked(index == MainDisplay);
        button->setCursor(Qt::PointingHandCursor);
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        button->setFont(ui::appFont(9, QFont::DemiBold));
        m_modeGroup->addButton(button, index);
        modeLayout->addWidget(button);
    }
    connect(m_modeGroup, &QButtonGroup::idClicked, this, &MirroringPage::updateMode);
    rootLayout->addWidget(modeBar);

    auto *scroll = new QScrollArea;
    scroll->setObjectName("MirroringScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    auto *content = new QWidget;
    content->setObjectName("MirroringContent");
    content->setMinimumWidth(760);
    auto *contentLayout = new QGridLayout(content);
    contentLayout->setContentsMargins(22, 18, 22, 18);
    contentLayout->setHorizontalSpacing(16);
    contentLayout->setVerticalSpacing(0);
    contentLayout->setColumnStretch(0, 3);
    contentLayout->setColumnStretch(1, 2);

    auto *mainColumn = new QVBoxLayout;
    mainColumn->setSpacing(16);
    auto *sideColumn = new QVBoxLayout;
    sideColumn->setSpacing(16);

    QVBoxLayout *imageLayout = nullptr;
    QFrame *imageCard = makeCard(QStringLiteral("icons/镜像/图像.png"), ui::text("图像"), &imageLayout);
    auto *imageFields = new QHBoxLayout;
    imageFields->setSpacing(10);
    m_maxSizeInput = makeNumberInput(ui::text("不限制"));
    m_maxFpsInput = makeNumberInput(ui::text("不限制"));
    m_maxSizeInput->setValidator(new QIntValidator(1, 16384, m_maxSizeInput));
    auto *fpsValidator = new QDoubleValidator(0.1, 1000.0, 2, m_maxFpsInput);
    fpsValidator->setNotation(QDoubleValidator::StandardNotation);
    m_maxFpsInput->setValidator(fpsValidator);
    imageFields->addWidget(makeField(ui::text("最大尺寸 (px)"), m_maxSizeInput));
    imageFields->addWidget(makeField(ui::text("最大 FPS"), m_maxFpsInput));
    imageLayout->addLayout(imageFields);
    auto *imageToggles = new QHBoxLayout;
    imageToggles->setSpacing(10);
    m_fullscreenCheck = makeToggle(ui::text("全屏"));
    m_screenOffCheck = makeToggle(ui::text("关闭设备物理屏幕"));
    imageToggles->addWidget(m_fullscreenCheck);
    imageToggles->addWidget(m_screenOffCheck);
    imageLayout->addLayout(imageToggles);
    mainColumn->addWidget(imageCard);

    QVBoxLayout *virtualLayout = nullptr;
    m_virtualCard = makeCard(QStringLiteral("icons/镜像/虚拟屏幕.png"), ui::text("虚拟屏幕"), &virtualLayout);
    auto *virtualFields = new QHBoxLayout;
    virtualFields->setSpacing(10);
    m_virtualWidthInput = makeNumberInput(ui::text("自动"));
    m_virtualHeightInput = makeNumberInput(ui::text("自动"));
    m_virtualDpiInput = makeNumberInput(ui::text("自动"));
    m_virtualWidthInput->setValidator(new QIntValidator(1, 16384, m_virtualWidthInput));
    m_virtualHeightInput->setValidator(new QIntValidator(1, 16384, m_virtualHeightInput));
    m_virtualDpiInput->setValidator(new QIntValidator(1, 10000, m_virtualDpiInput));
    virtualFields->addWidget(makeField(ui::text("宽度"), m_virtualWidthInput));
    virtualFields->addWidget(makeField(ui::text("高度"), m_virtualHeightInput));
    virtualFields->addWidget(makeField(QStringLiteral("DPI"), m_virtualDpiInput));
    virtualLayout->addLayout(virtualFields);
    m_virtualResizableCheck = makeToggle(ui::text("允许调整虚拟屏幕大小"), true);
    virtualLayout->addWidget(m_virtualResizableCheck);
    mainColumn->addWidget(m_virtualCard);

    QVBoxLayout *cameraLayout = nullptr;
    m_cameraCard = makeCard(QStringLiteral("icons/镜像/摄像头.png"), ui::text("摄像头"), &cameraLayout);
    auto *cameraSelectRow = new QHBoxLayout;
    m_cameraCombo = new QComboBox;
    m_cameraCombo->setObjectName("MirrorSelect");
    m_cameraCombo->setEditable(true);
    m_cameraCombo->addItem(ui::text("自动选择"), QString());
    auto *cameraRefresh = new QToolButton;
    cameraRefresh->setObjectName("MirrorIconButton");
    cameraRefresh->setText(QStringLiteral("↻"));
    cameraRefresh->setToolTip(ui::text("刷新摄像头列表"));
    cameraRefresh->setFixedSize(36, 36);
    cameraRefresh->setCursor(Qt::PointingHandCursor);
    connect(cameraRefresh, &QToolButton::clicked, this, [this] {
        m_camerasRequested = true;
        emit cameraListRequested();
    });
    cameraSelectRow->addWidget(makeField(ui::text("摄像头 ID"), m_cameraCombo), 1);
    cameraSelectRow->addWidget(cameraRefresh, 0, Qt::AlignBottom);
    cameraLayout->addLayout(cameraSelectRow);
    auto *cameraFields = new QHBoxLayout;
    cameraFields->setSpacing(10);
    m_cameraWidthInput = makeNumberInput(ui::text("自动"));
    m_cameraHeightInput = makeNumberInput(ui::text("自动"));
    m_cameraWidthInput->setValidator(new QIntValidator(1, 16384, m_cameraWidthInput));
    m_cameraHeightInput->setValidator(new QIntValidator(1, 16384, m_cameraHeightInput));
    cameraFields->addWidget(makeField(ui::text("宽度"), m_cameraWidthInput));
    cameraFields->addWidget(makeField(ui::text("高度"), m_cameraHeightInput));
    cameraLayout->addLayout(cameraFields);
    mainColumn->addWidget(m_cameraCard);

    QVBoxLayout *recordLayout = nullptr;
    QFrame *recordCard = makeCard(QStringLiteral("icons/镜像/录制.png"), ui::text("录制"), &recordLayout);
    m_recordCheck = makeToggle(ui::text("录制当前画面"));
    recordLayout->addWidget(m_recordCheck);
    auto *recordPathRow = new QHBoxLayout;
    m_recordPathInput = new QLineEdit;
    m_recordPathInput->setObjectName("MirrorInput");
    m_recordPathInput->setPlaceholderText(ui::text("选择 MP4 或 MKV 保存路径"));
    auto *recordBrowse = new QToolButton;
    recordBrowse->setObjectName("MirrorIconButton");
    recordBrowse->setText(QStringLiteral("▱"));
    recordBrowse->setToolTip(ui::text("选择录制文件"));
    recordBrowse->setFixedSize(36, 36);
    recordBrowse->setCursor(Qt::PointingHandCursor);
    connect(recordBrowse, &QToolButton::clicked, this, &MirroringPage::chooseRecordPath);
    recordPathRow->addWidget(m_recordPathInput, 1);
    recordPathRow->addWidget(recordBrowse);
    recordLayout->addLayout(recordPathRow);
    connect(m_recordCheck, &QCheckBox::toggled, m_recordPathInput, &QWidget::setEnabled);
    connect(m_recordCheck, &QCheckBox::toggled, recordBrowse, &QWidget::setEnabled);
    m_recordPathInput->setEnabled(false);
    recordBrowse->setEnabled(false);
    mainColumn->addWidget(recordCard);
    mainColumn->addStretch();

    QVBoxLayout *inputLayout = nullptr;
    QFrame *inputCard = makeCard(QStringLiteral("icons/镜像/输入与声音.png"), ui::text("输入与声音"), &inputLayout);
    m_viewOnlyCheck = makeToggle(ui::text("仅查看，不控制设备"));
    inputLayout->addWidget(m_viewOnlyCheck);
    m_audioCombo = new QComboBox;
    m_audioCombo->setObjectName("MirrorSelect");
    addComboOption(m_audioCombo, ui::text("默认"), QStringLiteral("default"));
    addComboOption(m_audioCombo, ui::text("无音频"), QStringLiteral("none"));
    addComboOption(m_audioCombo, ui::text("设备输出"), QStringLiteral("output"));
    addComboOption(m_audioCombo, ui::text("麦克风"), QStringLiteral("mic"));
    inputLayout->addWidget(makeField(ui::text("音频"), m_audioCombo));
    m_keyboardCombo = new QComboBox;
    m_keyboardCombo->setObjectName("MirrorSelect");
    m_mouseCombo = new QComboBox;
    m_mouseCombo->setObjectName("MirrorSelect");
    for (QComboBox *combo : {m_keyboardCombo, m_mouseCombo}) {
        addComboOption(combo, ui::text("默认"), QStringLiteral("default"));
        addComboOption(combo, QStringLiteral("SDK"), QStringLiteral("sdk"));
        addComboOption(combo, QStringLiteral("UHID"), QStringLiteral("uhid"));
        addComboOption(combo, QStringLiteral("AOA"), QStringLiteral("aoa"));
        addComboOption(combo, ui::text("禁用"), QStringLiteral("disabled"));
    }
    inputLayout->addWidget(makeField(ui::text("键盘"), m_keyboardCombo));
    inputLayout->addWidget(makeField(ui::text("鼠标"), m_mouseCombo));
    connect(m_viewOnlyCheck, &QCheckBox::toggled, this, [this](bool checked) {
        const bool enabled = !checked && m_modeGroup->checkedId() != Camera;
        m_keyboardCombo->setEnabled(enabled);
        m_mouseCombo->setEnabled(enabled);
    });
    sideColumn->addWidget(inputCard);

    QVBoxLayout *startupLayout = nullptr;
    m_startupCard = makeCard(QStringLiteral("icons/镜像/启动应用.png"), ui::text("启动应用"), &startupLayout);
    m_appCombo = new QComboBox;
    m_appCombo->setObjectName("MirrorSelect");
    m_appCombo->setEditable(true);
    m_appCombo->addItem(ui::text("不指定"), QString());
    startupLayout->addWidget(makeField(ui::text("应用或软件包名"), m_appCombo));
    sideColumn->addWidget(m_startupCard);

    QVBoxLayout *advancedLayout = nullptr;
    QFrame *advancedCard = makeCard(QStringLiteral("icons/镜像/高级参数.png"), ui::text("高级参数"), &advancedLayout);
    m_advancedInput = new QLineEdit;
    m_advancedInput->setObjectName("MirrorInput");
    m_advancedInput->setPlaceholderText(QStringLiteral("--video-bit-rate=8M"));
    advancedLayout->addWidget(makeField(ui::text("额外 scrcpy 参数"), m_advancedInput));
    sideColumn->addWidget(advancedCard);
    sideColumn->addStretch();

    contentLayout->addLayout(mainColumn, 0, 0);
    contentLayout->addLayout(sideColumn, 0, 1);
    scroll->setWidget(content);
    rootLayout->addWidget(scroll, 1);

    auto *footer = new QWidget;
    footer->setObjectName("MirroringFooter");
    footer->setFixedHeight(64);
    auto *footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(22, 8, 18, 8);
    footerLayout->setSpacing(10);
    m_footerInfoLabel = makeLabel(ui::text("设备音频需要 Android 11 或更高版本。"), 8);
    m_footerInfoLabel->setObjectName("MirroringFooterInfo");
    footerLayout->addWidget(m_footerInfoLabel, 1);
    m_advancedButton = new QPushButton(ui::text(">_  仅运行高级参数"));
    m_advancedButton->setObjectName("MirrorSecondaryButton");
    m_advancedButton->setCursor(Qt::PointingHandCursor);
    m_advancedButton->setVisible(false);
    connect(m_advancedButton, &QPushButton::clicked, this, &MirroringPage::launchAdvancedOnly);
    connect(m_advancedInput, &QLineEdit::textChanged, this, [this](const QString &text) {
        m_advancedButton->setVisible(!text.trimmed().isEmpty());
    });
    footerLayout->addWidget(m_advancedButton);
    m_launchButton = new QPushButton(ui::text("▣  使用 scrcpy 打开镜像"));
    m_launchButton->setObjectName("MirrorLaunchButton");
    m_launchButton->setCursor(Qt::PointingHandCursor);
    m_launchButton->setMinimumWidth(180);
    connect(m_launchButton, &QPushButton::clicked, this, &MirroringPage::launchConfigured);
    footerLayout->addWidget(m_launchButton);
    rootLayout->addWidget(footer);

    updateMode(MainDisplay);
    setDeviceConnected(false, QString());
}

void MirroringPage::activate()
{
    if (!m_connected) {
        return;
    }
    if (!m_appsRequested) {
        emit applicationListRequested();
    }
    if (m_modeGroup->checkedId() == Camera && !m_camerasRequested) {
        m_camerasRequested = true;
        emit cameraListRequested();
    }
}

void MirroringPage::setDeviceConnected(bool connected, const QString &serial)
{
    const bool changed = m_serial != serial;
    m_connected = connected;
    m_serial = connected ? serial : QString();
    if (changed) {
        m_appsRequested = false;
        m_camerasRequested = false;
        m_appCombo->clear();
        m_appCombo->addItem(ui::text("不指定"), QString());
        m_cameraCombo->clear();
        m_cameraCombo->addItem(ui::text("自动选择"), QString());
    }
    m_launchButton->setEnabled(connected || m_running);
    m_advancedButton->setEnabled(connected || m_running);
    if (connected) {
        setStatus(ui::text("已连接 · %1").arg(serial), QStringLiteral("#4f8f3d"));
    } else {
        setStatus(ui::text("等待设备连接"), QStringLiteral("#7b8798"));
    }
}

void MirroringPage::setMirrorRunning(bool running)
{
    m_running = running;
    m_launchButton->setText(running ? ui::text("■  停止镜像")
                                    : ui::text("▣  使用 scrcpy 打开镜像"));
    m_launchButton->setProperty("running", running);
    m_launchButton->style()->unpolish(m_launchButton);
    m_launchButton->style()->polish(m_launchButton);
    m_launchButton->setEnabled(m_connected || running);
    m_advancedButton->setEnabled(m_connected && !running);
    setStatus(running ? ui::text("scrcpy 正在运行 · %1").arg(m_serial)
                      : (m_connected ? ui::text("已连接 · %1").arg(m_serial)
                                     : ui::text("等待设备连接")),
              running || m_connected ? QStringLiteral("#4f8f3d")
                                     : QStringLiteral("#7b8798"));
}

void MirroringPage::setApplications(const QVector<AndroidAppSummary> &apps)
{
    m_appsRequested = true;
    const QString currentPackage = m_appCombo->currentData().toString().isEmpty()
        ? m_appCombo->currentText().trimmed()
        : m_appCombo->currentData().toString();
    m_appCombo->clear();
    m_appCombo->addItem(ui::text("不指定"), QString());
    for (const AndroidAppSummary &app : apps) {
        if (app.systemApp || app.uninstalled) {
            continue;
        }
        const QString name = app.displayName.trimmed().isEmpty() ? app.packageName
                                                                 : app.displayName.trimmed();
        const QString label = name == app.packageName
            ? app.packageName
            : QStringLiteral("%1 (%2)").arg(name, app.packageName);
        m_appCombo->addItem(label, app.packageName);
    }
    const int index = m_appCombo->findData(currentPackage);
    if (index >= 0) {
        m_appCombo->setCurrentIndex(index);
    }
}

void MirroringPage::setCameras(const QStringList &cameras)
{
    const QString current = m_cameraCombo->currentData().toString();
    m_cameraCombo->clear();
    m_cameraCombo->addItem(ui::text("自动选择"), QString());
    static const QRegularExpression idPattern(QStringLiteral("--camera-id=([^\\s]+)"));
    for (const QString &camera : cameras) {
        const QRegularExpressionMatch match = idPattern.match(camera);
        m_cameraCombo->addItem(camera, match.hasMatch() ? match.captured(1) : camera);
    }
    const int index = m_cameraCombo->findData(current);
    if (index >= 0) {
        m_cameraCombo->setCurrentIndex(index);
    }
    setStatus(cameras.isEmpty() ? ui::text("未发现可用摄像头")
                                : ui::text("已读取 %1 个摄像头").arg(cameras.size()),
              cameras.isEmpty() ? QStringLiteral("#c07824") : QStringLiteral("#4f8f3d"));
}

void MirroringPage::showError(const QString &message)
{
    setStatus(message.left(160), QStringLiteral("#d45b5b"));
    m_statusLabel->setToolTip(message);
}

void MirroringPage::updateMode(int mode)
{
    const bool virtualMode = mode == VirtualDisplay;
    const bool cameraMode = mode == Camera;
    m_virtualCard->setVisible(virtualMode);
    m_cameraCard->setVisible(cameraMode);
    m_startupCard->setVisible(!cameraMode);
    m_maxSizeInput->setEnabled(!cameraMode);
    m_screenOffCheck->setEnabled(!cameraMode);
    m_viewOnlyCheck->setEnabled(!cameraMode);
    m_keyboardCombo->setEnabled(!cameraMode && !m_viewOnlyCheck->isChecked());
    m_mouseCombo->setEnabled(!cameraMode && !m_viewOnlyCheck->isChecked());
    m_footerInfoLabel->setText(cameraMode
                                  ? ui::text("摄像头镜像需要 Android 12 或更高版本。")
                                  : ui::text("设备音频需要 Android 11 或更高版本。"));
    if (cameraMode && m_connected && !m_camerasRequested) {
        m_camerasRequested = true;
        emit cameraListRequested();
    }
}

void MirroringPage::chooseRecordPath()
{
    QString initial = m_recordPathInput->text().trimmed();
    if (initial.isEmpty()) {
        const QString directory = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
        initial = QStringLiteral("%1/mirror-%2.mp4")
                      .arg(directory,
                           QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    }
    const QString path = QFileDialog::getSaveFileName(this,
                                                      ui::text("选择录制文件"),
                                                      initial,
                                                      ui::text("视频文件 (*.mp4 *.mkv)"));
    if (!path.isEmpty()) {
        m_recordPathInput->setText(path);
    }
}

void MirroringPage::launchConfigured()
{
    if (m_running) {
        emit stopRequested();
        return;
    }
    QStringList arguments;
    if (buildArguments(&arguments)) {
        emit launchRequested(arguments);
    }
}

void MirroringPage::launchAdvancedOnly()
{
    if (m_running) {
        emit stopRequested();
        return;
    }
    const QString advanced = m_advancedInput->text().trimmed();
    if (advanced.isEmpty()) {
        showError(ui::text("请输入要运行的 scrcpy 高级参数。"));
        return;
    }
    emit launchRequested(QProcess::splitCommand(advanced));
}

bool MirroringPage::buildArguments(QStringList *arguments)
{
    if (!m_connected) {
        showError(ui::text("请先连接 Android 设备。"));
        return false;
    }

    const int mode = m_modeGroup->checkedId();
    if (mode == VirtualDisplay) {
        const QString width = m_virtualWidthInput->text().trimmed();
        const QString height = m_virtualHeightInput->text().trimmed();
        const QString dpi = m_virtualDpiInput->text().trimmed();
        if (width.isEmpty() != height.isEmpty()) {
            showError(ui::text("虚拟屏幕宽度和高度必须同时填写。"));
            return false;
        }
        if ((!width.isEmpty()
             && (!validInteger(width, 1, 16384) || !validInteger(height, 1, 16384)))
            || (!dpi.isEmpty() && !validInteger(dpi, 1, 10000))) {
            showError(ui::text("请输入有效的虚拟屏幕尺寸和 DPI。"));
            return false;
        }
        QString value;
        if (!width.isEmpty()) {
            value = width + QLatin1Char('x') + height;
        }
        if (!dpi.isEmpty()) {
            value += QLatin1Char('/') + dpi;
        }
        arguments->append(value.isEmpty() ? QStringLiteral("--new-display")
                                          : QStringLiteral("--new-display=%1").arg(value));
        if (m_virtualResizableCheck->isChecked()) {
            arguments->append(QStringLiteral("--flex-display"));
        }
    } else if (mode == Camera) {
        arguments->append(QStringLiteral("--video-source=camera"));
        QString cameraId = m_cameraCombo->currentData().toString().trimmed();
        if (cameraId.isEmpty() && m_cameraCombo->currentIndex() < 0) {
            cameraId = m_cameraCombo->currentText().trimmed();
        }
        const QRegularExpressionMatch idMatch = QRegularExpression(
            QStringLiteral("--camera-id=([^\\s]+)"))
                                                    .match(cameraId);
        if (idMatch.hasMatch()) {
            cameraId = idMatch.captured(1);
        }
        if (!cameraId.isEmpty()) {
            arguments->append(QStringLiteral("--camera-id=%1").arg(cameraId));
        }
        const QString width = m_cameraWidthInput->text().trimmed();
        const QString height = m_cameraHeightInput->text().trimmed();
        if (width.isEmpty() != height.isEmpty()) {
            showError(ui::text("摄像头宽度和高度必须同时填写。"));
            return false;
        }
        if (!width.isEmpty()
            && (!validInteger(width, 1, 16384) || !validInteger(height, 1, 16384))) {
            showError(ui::text("请输入有效的摄像头画面尺寸。"));
            return false;
        }
        if (!width.isEmpty()) {
            arguments->append(QStringLiteral("--camera-size=%1x%2").arg(width, height));
        }
    }

    if (m_fullscreenCheck->isChecked()) {
        arguments->append(QStringLiteral("--fullscreen"));
    }
    if (m_screenOffCheck->isChecked() && mode != Camera) {
        arguments->append(QStringLiteral("--turn-screen-off"));
    }
    if (m_viewOnlyCheck->isChecked() || mode == Camera) {
        arguments->append(QStringLiteral("--no-control"));
    }
    const QString maxSize = m_maxSizeInput->text().trimmed();
    if (!maxSize.isEmpty() && mode != Camera) {
        if (!validInteger(maxSize, 1, 16384)) {
            showError(ui::text("最大尺寸必须是 1 到 16384 之间的整数。"));
            return false;
        }
        arguments->append(QStringLiteral("--max-size=%1").arg(maxSize));
    }
    const QString maxFps = m_maxFpsInput->text().trimmed();
    if (!maxFps.isEmpty()) {
        if (!validNumber(maxFps, 0.1, 1000.0)) {
            showError(ui::text("最大 FPS 必须在 0.1 到 1000 之间。"));
            return false;
        }
        arguments->append(QStringLiteral("--max-fps=%1").arg(maxFps));
    }

    const QString audio = m_audioCombo->currentData().toString();
    if (audio == QStringLiteral("none")) {
        arguments->append(QStringLiteral("--no-audio"));
    } else if (audio != QStringLiteral("default")) {
        arguments->append(QStringLiteral("--audio-source=%1").arg(audio));
    }
    if (!m_viewOnlyCheck->isChecked() && mode != Camera) {
        const QString keyboard = m_keyboardCombo->currentData().toString();
        const QString mouse = m_mouseCombo->currentData().toString();
        if (keyboard != QStringLiteral("default")) {
            arguments->append(QStringLiteral("--keyboard=%1").arg(keyboard));
        }
        if (mouse != QStringLiteral("default")) {
            arguments->append(QStringLiteral("--mouse=%1").arg(mouse));
        }
    }

    if (mode != Camera) {
        QString packageName = m_appCombo->currentData().toString();
        if (packageName.isEmpty() && m_appCombo->currentIndex() < 0) {
            packageName = m_appCombo->currentText().trimmed();
        }
        if (!packageName.isEmpty()) {
            arguments->append(QStringLiteral("--start-app=%1").arg(packageName));
        }
    }
    if (m_recordCheck->isChecked()) {
        const QString recordPath = m_recordPathInput->text().trimmed();
        if (recordPath.isEmpty()) {
            showError(ui::text("启用录制后必须选择保存路径。"));
            return false;
        }
        arguments->append(QStringLiteral("--record=%1").arg(QFileInfo(recordPath).absoluteFilePath()));
    }

    const QString advanced = m_advancedInput->text().trimmed();
    if (!advanced.isEmpty()) {
        arguments->append(QProcess::splitCommand(advanced));
    }
    return true;
}

void MirroringPage::setStatus(const QString &text, const QString &color)
{
    m_statusLabel->setText(text);
    m_statusLabel->setStyleSheet(QStringLiteral("color:%1;").arg(color));
}
