#include "ui/windows/device_center_window.h"

#include "ui/common/app_preferences.h"
#include "ui/common/widget_helpers.h"

#include <QColor>
#include <QCloseEvent>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QFont>
#include <QHeaderView>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QResizeEvent>
#include <QSettings>
#include <QShowEvent>
#include <QSizePolicy>
#include <QSplitter>
#include <QTableWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

constexpr int kRefreshIntervalMs = 3000;

QFrame *makeSeparator()
{
    auto *separator = new QFrame;
    separator->setObjectName("DeviceCenterSeparator");
    separator->setFrameShape(QFrame::VLine);
    separator->setFixedHeight(24);
    return separator;
}

QToolButton *makeToolButton(QWidget *parent,
                            const QString &iconPath,
                            const QString &toolTip)
{
    auto *button = new QToolButton(parent);
    button->setObjectName("DeviceCenterToolButton");
    button->setIcon(ui::imageIcon(iconPath));
    button->setIconSize(QSize(18, 18));
    button->setFixedSize(36, 34);
    button->setCursor(Qt::PointingHandCursor);
    button->setToolTip(toolTip);
    return button;
}

} // namespace

DeviceCenterWindow::DeviceCenterWindow(DeviceCenterService *service, QWidget *parent)
    : QMainWindow(parent)
    , m_service(service)
{
    setObjectName("DeviceCenterWindow");
    setAttribute(Qt::WA_DeleteOnClose, false);
    setMinimumSize(960, 640);
    resize(1180, 760);
    buildUi();
    applyLanguage();

    connect(&ui::AppPreferences::instance(),
            &ui::AppPreferences::languageChanged,
            this,
            [this] { applyLanguage(); });
    connect(m_service,
            &DeviceCenterService::refreshStarted,
            this,
            [this] { m_refreshButton->setEnabled(false); });
    connect(m_service,
            &DeviceCenterService::devicesUpdated,
            this,
            &DeviceCenterWindow::populateDevices);
    connect(m_service,
            &DeviceCenterService::operationStarted,
            this,
            [this](const QString &) {
                m_commandBusy = true;
                updateSelectionState();
            });
    connect(m_service,
            &DeviceCenterService::operationFinished,
            this,
            [this](bool success, const QString &label, const QString &detail) {
                m_commandBusy = false;
                m_refreshButton->setEnabled(true);
                updateSelectionState();
                if (!success && isVisible()) {
                    QMessageBox::warning(this, label, detail);
                }
            });
    connect(m_service,
            &DeviceCenterService::screenshotReady,
            this,
            [this](const QString &deviceId, const QImage &image) {
                if (deviceId != m_selectedDeviceId) {
                    return;
                }
                m_previewPixmap = QPixmap::fromImage(image);
                updatePreviewPixmap();
            });
    connect(m_service,
            &DeviceCenterService::screenshotFailed,
            this,
            [this](const QString &deviceId, const QString &detail) {
                if (deviceId == m_selectedDeviceId) {
                    showPreviewMessage(detail);
                }
            });

    m_refreshTimer = new QTimer(this);
    m_refreshTimer->setInterval(kRefreshIntervalMs);
    connect(m_refreshTimer,
            &QTimer::timeout,
            m_service,
            &DeviceCenterService::refreshDevices);

    QSettings settings;
    const QByteArray geometry = settings.value(QStringLiteral("deviceCenter/geometry")).toByteArray();
    if (!geometry.isEmpty()) {
        restoreGeometry(geometry);
    }
    const QByteArray splitterState = settings.value(
        QStringLiteral("deviceCenter/splitterState"))
                                         .toByteArray();
    if (!splitterState.isEmpty()) {
        m_splitter->restoreState(splitterState);
    } else {
        m_splitter->setSizes({430, 280});
    }
}

void DeviceCenterWindow::setActiveDeviceSerial(const QString &serial)
{
    if (m_activeDeviceSerial == serial) {
        return;
    }
    m_activeDeviceSerial = serial;
    updateActiveRow();
}

void DeviceCenterWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    m_service->refreshDevices();
    m_refreshTimer->start();
}

void DeviceCenterWindow::hideEvent(QHideEvent *event)
{
    m_refreshTimer->stop();
    saveWindowState();
    QMainWindow::hideEvent(event);
}

void DeviceCenterWindow::closeEvent(QCloseEvent *event)
{
    saveWindowState();
    QMainWindow::closeEvent(event);
}

void DeviceCenterWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updatePreviewPixmap();
}

void DeviceCenterWindow::buildUi()
{
    auto *root = new QWidget;
    root->setObjectName("DeviceCenterRoot");
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *toolbar = new QWidget;
    toolbar->setObjectName("DeviceCenterToolbar");
    toolbar->setFixedHeight(52);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(8, 7, 8, 7);
    toolbarLayout->setSpacing(8);

    m_ipInput = new QLineEdit;
    m_ipInput->setObjectName("DeviceCenterIpInput");
    m_ipInput->setFixedWidth(190);
    m_ipInput->setClearButtonEnabled(true);
    toolbarLayout->addWidget(m_ipInput);

    m_portInput = new QLineEdit;
    m_portInput->setObjectName("DeviceCenterPortInput");
    m_portInput->setFixedWidth(86);
    m_portInput->setValidator(new QIntValidator(1, 65535, m_portInput));
    toolbarLayout->addWidget(m_portInput);

    m_connectButton = new QPushButton;
    m_connectButton->setObjectName("DeviceCenterCommandButton");
    m_connectButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_connectButton);
    toolbarLayout->addWidget(makeSeparator());

    m_pairButton = new QPushButton;
    m_pairButton->setObjectName("DeviceCenterCommandButton");
    m_pairButton->setCursor(Qt::PointingHandCursor);
    toolbarLayout->addWidget(m_pairButton);
    toolbarLayout->addStretch();

    m_wirelessButton = makeToolButton(
        toolbar,
        QStringLiteral("icons/设备中心/无线模式.png"),
        QString());
    toolbarLayout->addWidget(m_wirelessButton);
    m_disconnectButton = makeToolButton(
        toolbar,
        QStringLiteral("icons/设备中心/断开设备.png"),
        QString());
    toolbarLayout->addWidget(m_disconnectButton);
    m_deleteButton = makeToolButton(
        toolbar,
        QStringLiteral("icons/设备中心/删除离线设备.png"),
        QString());
    toolbarLayout->addWidget(m_deleteButton);
    toolbarLayout->addWidget(makeSeparator());

    m_filterInput = new QLineEdit;
    m_filterInput->setObjectName("DeviceCenterFilterInput");
    m_filterInput->setFixedWidth(190);
    m_filterInput->setClearButtonEnabled(true);
    toolbarLayout->addWidget(m_filterInput);
    m_refreshButton = makeToolButton(
        toolbar,
        QStringLiteral("icons/设备中心/刷新.png"),
        QString());
    toolbarLayout->addWidget(m_refreshButton);
    rootLayout->addWidget(toolbar);

    m_splitter = new QSplitter(Qt::Vertical);
    m_splitter->setObjectName("DeviceCenterSplitter");
    m_splitter->setChildrenCollapsible(false);

    m_deviceTable = new QTableWidget;
    m_deviceTable->setObjectName("DeviceCenterTable");
    m_deviceTable->setColumnCount(5);
    m_deviceTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_deviceTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_deviceTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_deviceTable->setAlternatingRowColors(true);
    m_deviceTable->setShowGrid(true);
    m_deviceTable->setSortingEnabled(true);
    m_deviceTable->verticalHeader()->setVisible(false);
    m_deviceTable->verticalHeader()->setDefaultSectionSize(34);
    m_deviceTable->horizontalHeader()->setHighlightSections(false);
    m_deviceTable->horizontalHeader()->setStretchLastSection(false);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    m_deviceTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Interactive);
    m_deviceTable->setColumnWidth(0, 190);
    m_deviceTable->setColumnWidth(1, 170);
    m_deviceTable->setColumnWidth(4, 110);
    m_splitter->addWidget(m_deviceTable);

    auto *preview = new QWidget;
    preview->setObjectName("DeviceCenterPreview");
    auto *previewLayout = new QVBoxLayout(preview);
    previewLayout->setContentsMargins(16, 16, 16, 16);
    m_previewLabel = new QLabel;
    m_previewLabel->setObjectName("DeviceCenterPreviewLabel");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    previewLayout->addWidget(m_previewLabel, 1);
    m_splitter->addWidget(preview);
    rootLayout->addWidget(m_splitter, 1);
    setCentralWidget(root);

    connect(m_ipInput, &QLineEdit::textChanged, this, [this] { updateSelectionState(); });
    connect(m_connectButton, &QPushButton::clicked, this, [this] {
        bool portOk = false;
        const int port = m_portInput->text().toInt(&portOk);
        m_service->connectDevice(m_ipInput->text(), portOk ? port : 0);
    });
    connect(m_pairButton,
            &QPushButton::clicked,
            this,
            &DeviceCenterWindow::showPairDialog);
    connect(m_wirelessButton, &QToolButton::clicked, this, [this] {
        if (const DeviceCenterDevice *device = selectedDevice()) {
            m_service->enableWireless(device->id);
        }
    });
    connect(m_disconnectButton, &QToolButton::clicked, this, [this] {
        if (const DeviceCenterDevice *device = selectedDevice()) {
            m_service->disconnectDevice(device->id);
        }
    });
    connect(m_deleteButton, &QToolButton::clicked, this, [this] {
        if (const DeviceCenterDevice *device = selectedDevice()) {
            m_service->removeRememberedDevice(device->id);
        }
    });
    connect(m_filterInput,
            &QLineEdit::textChanged,
            this,
            &DeviceCenterWindow::applyFilter);
    connect(m_refreshButton,
            &QToolButton::clicked,
            m_service,
            &DeviceCenterService::refreshDevices);
    connect(m_deviceTable,
            &QTableWidget::itemSelectionChanged,
            this,
            [this] {
                const QList<QTableWidgetItem *> selectedItems = m_deviceTable->selectedItems();
                m_selectedDeviceId = selectedItems.isEmpty()
                    ? QString()
                    : selectedItems.first()->data(Qt::UserRole).toString();
                updateSelectionState();
                requestSelectedScreenshot();
            });
    connect(m_deviceTable,
            &QTableWidget::doubleClicked,
            this,
            [this](const QModelIndex &) {
                const DeviceCenterDevice *device = selectedDevice();
                if (device == nullptr || !device->online) {
                    return;
                }
                setActiveDeviceSerial(device->id);
                emit deviceActivationRequested(device->id);
            });
}

void DeviceCenterWindow::applyLanguage()
{
    setWindowTitle(ui::text("设备管理器"));
    m_ipInput->setPlaceholderText(ui::text("IP 地址"));
    m_portInput->setPlaceholderText(ui::text("端口"));
    m_connectButton->setText(ui::text("连接"));
    m_pairButton->setText(ui::text("配对"));
    m_wirelessButton->setToolTip(ui::text("无线模式"));
    m_disconnectButton->setToolTip(ui::text("断开设备"));
    m_deleteButton->setToolTip(ui::text("删除离线设备"));
    m_filterInput->setPlaceholderText(ui::text("过滤"));
    m_refreshButton->setToolTip(ui::text("刷新"));
    m_deviceTable->setHorizontalHeaderLabels({QStringLiteral("ID"),
                                              ui::text("序列号"),
                                              ui::text("名称"),
                                              ui::text("Android 版本"),
                                              ui::text("状态")});
    populateDevices(m_devices);
    if (m_selectedDeviceId.isEmpty()) {
        showPreviewMessage(ui::text("未选择设备"));
    }
    ui::AppPreferences::instance().applyFont(this);
}

void DeviceCenterWindow::populateDevices(const QList<DeviceCenterDevice> &devices)
{
    m_devices = devices;
    const QString selectedId = m_selectedDeviceId;
    m_deviceTable->setSortingEnabled(false);
    m_deviceTable->clearContents();
    m_deviceTable->setRowCount(m_devices.size());

    int selectedRow = -1;
    for (qsizetype row = 0; row < m_devices.size(); ++row) {
        const DeviceCenterDevice &device = m_devices.at(row);
        const QString version = device.androidVersion.isEmpty()
            ? QString()
            : device.sdkVersion.isEmpty()
                ? QStringLiteral("Android %1").arg(device.androidVersion)
                : QStringLiteral("Android %1 (API %2)")
                      .arg(device.androidVersion, device.sdkVersion);
        const QStringList values = {device.id,
                                    device.serialNumber,
                                    device.name,
                                    version,
                                    statusText(device)};
        for (int column = 0; column < values.size(); ++column) {
            auto *item = new QTableWidgetItem(values.at(column));
            item->setData(Qt::UserRole, device.id);
            item->setToolTip(values.at(column));
            m_deviceTable->setItem(row, column, item);
        }
        if (device.id == selectedId) {
            selectedRow = static_cast<int>(row);
        }
    }
    m_deviceTable->setSortingEnabled(true);
    applyFilter();
    if (selectedRow >= 0) {
        for (int row = 0; row < m_deviceTable->rowCount(); ++row) {
            const QTableWidgetItem *item = m_deviceTable->item(row, 0);
            if (item != nullptr && item->data(Qt::UserRole).toString() == selectedId) {
                m_deviceTable->selectRow(row);
                break;
            }
        }
    } else {
        m_selectedDeviceId.clear();
        m_previewPixmap = QPixmap();
        showPreviewMessage(ui::text("未选择设备"));
    }
    updateActiveRow();
    updateSelectionState();
    m_refreshButton->setEnabled(true);
}

void DeviceCenterWindow::applyFilter()
{
    const QString filter = m_filterInput->text().trimmed();
    for (int row = 0; row < m_deviceTable->rowCount(); ++row) {
        bool matches = filter.isEmpty();
        for (int column = 0; !matches && column < m_deviceTable->columnCount(); ++column) {
            const QTableWidgetItem *item = m_deviceTable->item(row, column);
            matches = item != nullptr && item->text().contains(filter, Qt::CaseInsensitive);
        }
        m_deviceTable->setRowHidden(row, !matches);
    }
}

void DeviceCenterWindow::updateSelectionState()
{
    const DeviceCenterDevice *device = selectedDevice();
    const bool online = device != nullptr && device->online;
    const bool remote = device != nullptr && device->remote;
    m_connectButton->setEnabled(!m_commandBusy && !m_ipInput->text().trimmed().isEmpty());
    m_pairButton->setEnabled(!m_commandBusy);
    m_wirelessButton->setEnabled(!m_commandBusy && online && !remote);
    m_disconnectButton->setEnabled(!m_commandBusy && online && remote);
    m_deleteButton->setEnabled(!m_commandBusy && device != nullptr && remote && !online);
}

void DeviceCenterWindow::updateActiveRow()
{
    for (int row = 0; row < m_deviceTable->rowCount(); ++row) {
        QTableWidgetItem *idItem = m_deviceTable->item(row, 0);
        if (idItem == nullptr) {
            continue;
        }
        const bool active = idItem->data(Qt::UserRole).toString() == m_activeDeviceSerial;
        for (int column = 0; column < m_deviceTable->columnCount(); ++column) {
            QTableWidgetItem *item = m_deviceTable->item(row, column);
            if (item == nullptr) {
                continue;
            }
            QFont font = item->font();
            font.setBold(active);
            item->setFont(font);
            item->setForeground(active ? QColor(QStringLiteral("#0066cc"))
                                       : QColor(QStringLiteral("#1d1d1f")));
        }
        idItem->setToolTip(active ? ui::text("当前主设备") : idItem->text());
    }
}

void DeviceCenterWindow::showPairDialog()
{
    QDialog dialog(this);
    dialog.setObjectName("DeviceCenterPairDialog");
    dialog.setWindowTitle(ui::text("设备配对"));
    dialog.setModal(true);
    dialog.setMinimumWidth(400);

    auto *layout = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    auto *ipInput = new QLineEdit;
    ipInput->setPlaceholderText(ui::text("IP 地址"));
    auto *portInput = new QLineEdit;
    portInput->setValidator(new QIntValidator(1, 65535, portInput));
    portInput->setPlaceholderText(ui::text("端口"));
    auto *codeInput = new QLineEdit;
    codeInput->setValidator(new QIntValidator(0, 999999, codeInput));
    codeInput->setMaxLength(6);
    codeInput->setPlaceholderText(ui::text("6 位配对码"));
    form->addRow(ui::text("IP 地址"), ipInput);
    form->addRow(ui::text("端口"), portInput);
    form->addRow(ui::text("配对码"), codeInput);
    layout->addLayout(form);

    auto *buttons = new QDialogButtonBox;
    auto *pairButton = buttons->addButton(ui::text("配对"), QDialogButtonBox::AcceptRole);
    auto *cancelButton = buttons->addButton(ui::text("取消"), QDialogButtonBox::RejectRole);
    pairButton->setObjectName("DeviceCenterPrimaryButton");
    cancelButton->setObjectName("DeviceCenterSecondaryButton");
    pairButton->setAutoDefault(false);
    cancelButton->setAutoDefault(false);
    pairButton->setEnabled(false);
    layout->addWidget(buttons);

    const auto updatePairButton = [ipInput, portInput, codeInput, pairButton] {
        bool portOk = false;
        const int port = portInput->text().toInt(&portOk);
        pairButton->setEnabled(!ipInput->text().trimmed().isEmpty() && portOk && port > 0
                               && codeInput->text().size() == 6);
    };
    connect(ipInput, &QLineEdit::textChanged, &dialog, updatePairButton);
    connect(portInput, &QLineEdit::textChanged, &dialog, updatePairButton);
    connect(codeInput, &QLineEdit::textChanged, &dialog, updatePairButton);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(pairButton, &QPushButton::clicked, &dialog, [&] {
        m_service->pairDevice(ipInput->text(),
                              portInput->text().toInt(),
                              codeInput->text());
        dialog.accept();
    });
    dialog.exec();
}

void DeviceCenterWindow::requestSelectedScreenshot()
{
    const DeviceCenterDevice *device = selectedDevice();
    m_previewPixmap = QPixmap();
    if (device == nullptr) {
        showPreviewMessage(ui::text("未选择设备"));
        return;
    }
    if (!device->online) {
        showPreviewMessage(ui::text("设备离线"));
        return;
    }
    showPreviewMessage(ui::text("正在获取屏幕截图…"));
    m_service->captureScreenshot(device->id);
}

void DeviceCenterWindow::showPreviewMessage(const QString &message)
{
    m_previewPixmap = QPixmap();
    m_previewLabel->setPixmap(QPixmap());
    m_previewLabel->setText(message);
}

void DeviceCenterWindow::updatePreviewPixmap()
{
    if (m_previewPixmap.isNull() || m_previewLabel == nullptr) {
        return;
    }
    const QSize available = m_previewLabel->size() - QSize(12, 12);
    if (available.width() <= 0 || available.height() <= 0) {
        return;
    }
    m_previewLabel->setText(QString());
    m_previewLabel->setPixmap(
        m_previewPixmap.scaled(available, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void DeviceCenterWindow::saveWindowState()
{
    QSettings settings;
    settings.setValue(QStringLiteral("deviceCenter/geometry"), saveGeometry());
    settings.setValue(QStringLiteral("deviceCenter/splitterState"), m_splitter->saveState());
}

const DeviceCenterDevice *DeviceCenterWindow::selectedDevice() const
{
    for (const DeviceCenterDevice &device : m_devices) {
        if (device.id == m_selectedDeviceId) {
            return &device;
        }
    }
    return nullptr;
}

QString DeviceCenterWindow::statusText(const DeviceCenterDevice &device)
{
    if (device.online) {
        return ui::text("在线");
    }
    if (device.state == QStringLiteral("unauthorized")) {
        return ui::text("未授权");
    }
    if (device.state == QStringLiteral("sideload")) {
        return ui::text("侧载模式");
    }
    return ui::text("离线");
}
