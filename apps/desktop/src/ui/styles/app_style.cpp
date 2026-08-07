#include "ui/styles/app_style.h"

namespace ui {

QString appStyleSheet()
{
    return QString::fromUtf8(R"(
        QWidget {
            color: #172033;
            background: transparent;
        }
        QMainWindow { background: #f7faff; }
        QFrame#Root { background: #f7faff; }
        QFrame#Header {
            background: #fbfdff;
            border-bottom: 1px solid #dfe7f3;
        }
        QFrame#Sidebar {
            background: #fbfdff;
            border-right: 1px solid #dfe7f3;
        }
        QScrollArea#SidebarNavigationScroll,
        QWidget#SidebarNavigation {
            background: #fbfdff;
            border: none;
        }
        QScrollArea#SidebarNavigationScroll QScrollBar:vertical {
            width: 7px;
            background: transparent;
            margin: 3px 1px;
        }
        QScrollArea#SidebarNavigationScroll QScrollBar::handle:vertical {
            min-height: 24px;
            background: #cbd4e1;
            border-radius: 3px;
        }
        QScrollArea#SidebarNavigationScroll QScrollBar::add-line:vertical,
        QScrollArea#SidebarNavigationScroll QScrollBar::sub-line:vertical {
            height: 0;
        }
        QFrame#Panel, QFrame#DevicePane, QFrame#ChatPane, QFrame#PhoneHost {
            background: #ffffff;
            border: 1px solid #dfe7f3;
            border-radius: 8px;
        }
        QFrame#DevicePane, QFrame#ChatPane { background: #ffffff; }
        QFrame#PhoneHost { background: #fbfdff; }
        QFrame#PhoneControls {
            background: #ffffff;
            border: none;
        }
        QPushButton#DeviceSelector {
            background: #ffffff;
            border: 1px solid #d9e4f4;
            border-radius: 8px;
            padding: 0;
            text-align: left;
        }
        QPushButton#DeviceSelector:hover { background: #f7faff; border-color: #bcd0ed; }
        QPushButton#DeviceSelector:pressed { background: #edf4ff; border-color: #9fbee8; }
        QFrame#NavItem, QFrame#PlainRow {
            background: transparent;
            border: none;
            border-radius: 8px;
        }
        QFrame#NavItemSelected {
            background: #eaf2ff;
            border: none;
            border-radius: 8px;
        }
        QPushButton#WorkspaceNavButton {
            background: transparent;
            border: none;
            border-radius: 8px;
            color: #293243;
            text-align: left;
            padding: 0 14px;
        }
        QPushButton#WorkspaceNavButton:hover { background: #f1f5fb; }
        QPushButton#WorkspaceNavButton[active="true"] {
            background: #eaf2ff;
            color: #2f6df6;
        }
        QFrame#Divider {
            background: #dfe7f3;
            border: none;
        }
        QPushButton#IconButton {
            background: transparent;
            border: none;
            border-radius: 8px;
            color: #172033;
        }
        QPushButton#IconButton:hover { background: #eef5ff; }
        QFrame#AssistantBubble, QFrame#AssistantBubbleLarge {
            background: #f2f5fa;
            border: 1px solid #edf2f8;
            border-radius: 8px;
        }
        QFrame#UserBubble {
            background: #dceaff;
            border: 1px solid #c9dcfb;
            border-radius: 8px;
        }
        QFrame#StepRow {
            background: rgba(255, 255, 255, 150);
            border: none;
            border-radius: 6px;
        }
        QFrame#AttachmentChip {
            background: #ffffff;
            border: 1px solid #dbe5f4;
            border-radius: 8px;
        }
        QFrame#ExcelIcon {
            background: #40b95f;
            border-radius: 6px;
        }
        QFrame#PdfIcon {
            background: #4b83f6;
            border-radius: 6px;
        }
        QFrame#InputBox {
            background: #ffffff;
            border: 1px solid #c7d8f1;
            border-radius: 8px;
        }
        QPlainTextEdit#PromptInput {
            background: #ffffff;
            border: none;
            color: #293243;
            selection-background-color: #dceaff;
        }
        QPushButton#SendButton {
            background: #2f6df6;
            border: none;
            border-radius: 8px;
            color: white;
        }
        QPushButton#SendButton:hover { background: #245fe0; }
        QScrollArea#ChatScroll {
            background: transparent;
            border: none;
        }
        QWidget#DeviceControlPage { background: #f7faff; }
        QWidget#PackageManagerPage { background: #f7faff; }
        QWidget#RecoveryPage { background: #f7faff; }
        QWidget#FilesPage { background: #f7faff; }
        QWidget#AppsPage { background: #f7faff; }
        QScrollArea#DeviceControlScroll {
            background: transparent;
            border: none;
        }
        QFrame#CommandCategory {
            background: #ffffff;
            border: 1px solid #dfe7f3;
            border-radius: 8px;
        }
        QFrame#CommandCategoryHeader {
            background: #ffffff;
            border: none;
            border-radius: 8px;
        }
        QFrame#CommandCategoryIcon {
            background: #f7f9fc;
            border: 1px solid #cfd8e6;
            border-radius: 6px;
        }
        QPushButton#CommandCategoryButton {
            background: transparent;
            border: none;
            color: #172033;
            text-align: left;
            padding: 4px 0;
        }
        QPushButton#CommandCategoryButton:hover { color: #2f6df6; }
        QToolButton#CommandCategoryToggle {
            background: #f3f6fa;
            border: 1px solid #dfe7f3;
            border-radius: 8px;
            color: #596579;
        }
        QToolButton#CommandCategoryToggle:hover {
            background: #eaf2ff;
            color: #2f6df6;
        }
        QFrame#CommandCategoryContent {
            background: #fbfcfe;
            border: none;
            border-top: 1px solid #edf1f7;
        }
        QPushButton#DeviceCommandButton {
            background: #ffffff;
            border: 1px solid #dbe4f0;
            border-radius: 7px;
            color: #293243;
            padding: 6px 10px;
        }
        QPushButton#DeviceCommandButton:hover {
            background: #eef5ff;
            border-color: #9bbcf7;
            color: #245fe0;
        }
        QPushButton#DeviceCommandButton[danger="true"] {
            background: #fffaf0;
            border-color: #ead8ad;
            color: #765618;
        }
        QPushButton#DeviceCommandButton:disabled {
            background: #f2f4f7;
            border-color: #e4e8ee;
            color: #aab3c2;
        }
        QFrame#PackageToolbar, QFrame#PackageListPanel, QFrame#PackageDetailPanel {
            background: #ffffff;
            border: 1px solid #dfe7f3;
            border-radius: 8px;
        }
        QLineEdit#PackageSearchInput {
            background: #ffffff;
            border: 1px solid #c7d8f1;
            border-radius: 6px;
            color: #293243;
            min-height: 30px;
            padding: 0 10px;
        }
        QLineEdit#PackageSearchInput:focus { border-color: #7da7ef; }
        QCheckBox#PackageFilter {
            color: #596579;
            spacing: 5px;
        }
        QCheckBox#PackageFilter::indicator {
            width: 14px;
            height: 14px;
            border: 1px solid #b7c7df;
            border-radius: 3px;
            background: #ffffff;
        }
        QCheckBox#PackageFilter::indicator:checked {
            background: #2f6df6;
            border-color: #2f6df6;
        }
        QToolButton#PackageRefreshButton {
            background: #f3f6fa;
            border: 1px solid #dfe7f3;
            border-radius: 6px;
            color: #2f6df6;
        }
        QToolButton#PackageRefreshButton:hover { background: #eaf2ff; }
        QToolButton#PackageRefreshButton:disabled {
            color: #aab3c2;
            background: #f4f6f8;
        }
        QListWidget#PackageList {
            background: #fbfdff;
            border: 1px solid #e0e8f4;
            border-radius: 6px;
            color: #293243;
            outline: none;
            padding: 4px;
        }
        QListWidget#PackageList::item {
            min-height: 28px;
            border-radius: 4px;
            padding: 2px 8px;
        }
        QListWidget#PackageList::item:hover { background: #eef5ff; }
        QListWidget#PackageList::item:selected {
            background: #dceaff;
            color: #245fe0;
        }
        QPushButton#PackageActionButton {
            background: #ffffff;
            border: 1px solid #cbd9ec;
            border-radius: 6px;
            color: #36506f;
            padding: 0 10px;
        }
        QPushButton#PackageActionButton:hover {
            background: #eef5ff;
            border-color: #9bbcf7;
            color: #245fe0;
        }
        QPushButton#PackageActionButton[danger="true"] {
            color: #a24b42;
            border-color: #efc8c1;
            background: #fff9f7;
        }
        QPushButton#PackageActionButton[danger="true"]:hover {
            background: #fff0ed;
            border-color: #de8f83;
        }
        QPushButton#PackageActionButton:disabled {
            background: #f2f4f7;
            border-color: #e4e8ee;
            color: #aab3c2;
        }
    )") + QString::fromUtf8(R"(
        QScrollArea#PackageCatalogScroll {
            background: transparent;
            border: none;
        }
        QFrame#PackageCategoryCard {
            background: #ffffff;
            border: 1px solid #e1e7ef;
            border-radius: 7px;
        }
        QFrame#PackageCategoryCard:hover {
            background: #fbfdff;
            border-color: #c8d8ee;
        }
        QFrame#PackageCategoryIcon {
            background: #ffffff;
            border: 1px solid #cfd8e6;
            border-radius: 6px;
        }
        QPushButton#PackageCategoryOpenButton {
            background: transparent;
            border: none;
            color: #172033;
            text-align: left;
            padding: 8px 0;
        }
        QPushButton#PackageCategoryOpenButton:hover { color: #2f6df6; }
        QToolButton#PackageCopyButton, QToolButton#PackageBackButton {
            background: transparent;
            border: none;
            border-radius: 6px;
            color: #596579;
        }
        QToolButton#PackageCopyButton:hover, QToolButton#PackageBackButton:hover {
            background: #eaf2ff;
            color: #2f6df6;
        }
        QPushButton#PackageResultActionButton {
            background: #fff9f7;
            border: 1px solid #efc8c1;
            border-radius: 6px;
            color: #a24b42;
            padding: 0 12px;
        }
        QPushButton#PackageResultActionButton:hover {
            background: #fff0ed;
            border-color: #de8f83;
        }
        QPushButton#PackageResultActionButton:disabled {
            background: #f2f4f7;
            border-color: #e4e8ee;
            color: #aab3c2;
        }
        QListWidget#PackageResultList {
            background: #ffffff;
            border: 1px solid #dfe7f3;
            border-radius: 7px;
            color: #293243;
            outline: none;
            padding: 4px;
        }
        QListWidget#PackageResultList::item {
            min-height: 30px;
            border-radius: 4px;
            padding: 2px 10px;
        }
        QListWidget#PackageResultList::item:hover { background: #eef5ff; }
        QListWidget#PackageResultList::item:selected {
            background: #dceaff;
            color: #245fe0;
        }
        QFrame#PackageInstallPanel {
            background: #ffffff;
            border: 1px solid #dfe7f3;
            border-radius: 7px;
        }
        QFrame#PackageInstallNotice {
            background: #f1faf6;
            border: 1px solid #bee4d3;
            border-radius: 7px;
        }
        QLineEdit#PackageInstallFilePath {
            background: #fbfdff;
            border: 1px solid #c7d8f1;
            border-radius: 6px;
            color: #293243;
            min-height: 32px;
            padding: 0 10px;
        }
        QToolButton#PackageInstallSelectButton {
            background: #ffffff;
            border: 1px solid #cbd9ec;
            border-radius: 6px;
            color: #36506f;
        }
        QToolButton#PackageInstallSelectButton:hover {
            background: #eaf2ff;
            border-color: #9bbcf7;
            color: #2f6df6;
        }
        QToolButton#PackageInstallSelectButton:disabled {
            background: #f2f4f7;
            border-color: #e4e8ee;
            color: #aab3c2;
        }
        QPushButton#PackageInstallStartButton {
            background: #2f6df6;
            border: none;
            border-radius: 6px;
            color: #ffffff;
            padding: 0 14px;
        }
        QPushButton#PackageInstallStartButton:hover { background: #245fe0; }
        QPushButton#PackageInstallStartButton:disabled {
            background: #e8edf5;
            color: #9aa5b5;
        }
        QProgressBar#PackageInstallProgress {
            background: #e8edf5;
            border: none;
            border-radius: 5px;
            min-height: 10px;
            max-height: 10px;
        }
        QProgressBar#PackageInstallProgress::chunk {
            background: #3aa675;
            border-radius: 5px;
        }
    )") + QString::fromUtf8(R"(
        QWidget#TerminalPage, QStackedWidget#TerminalStack,
        QWidget#TerminalViewport {
            background: #0c0c0c;
            border: none;
        }
        QWidget#TerminalToolbar {
            background: #2d2d2d;
            border: none;
            border-bottom: 1px solid #3b3b3b;
        }
        QTabBar#TerminalTabs {
            background: transparent;
            border: none;
        }
        QTabBar#TerminalTabs::tab {
            background: transparent;
            border: none;
            border-right: 1px solid #3b3b3b;
            color: #d8d8d8;
            min-width: 128px;
            min-height: 42px;
            padding: 0 12px;
        }
        QTabBar#TerminalTabs::tab:hover {
            background: #383838;
            color: #ffffff;
        }
        QTabBar#TerminalTabs::tab:selected {
            background: #0c0c0c;
            color: #ffffff;
            border-bottom: 2px solid #3b78ff;
        }
        QAbstractButton#TerminalTabCloseButton {
            background: transparent;
            border: none;
            color: #b8b8b8;
            font-size: 15px;
        }
        QAbstractButton#TerminalTabCloseButton:hover { color: #ff7b72; }
        QToolButton#TerminalToolButton {
            background: transparent;
            border: none;
            border-radius: 6px;
            color: #f2f2f2;
        }
        QToolButton#TerminalToolButton:hover {
            background: #444444;
            color: #ffffff;
        }
        QToolButton#TerminalToolButton:disabled {
            background: transparent;
            color: #777777;
        }
        QPlainTextEdit#TerminalViewport {
            background: #0c0c0c;
            border: none;
            color: #f2f2f2;
            padding: 8px 6px;
            selection-background-color: #264f78;
            selection-color: #ffffff;
        }
        QMenu {
            background: #ffffff;
            border: 1px solid #d6e0ee;
            color: #293243;
            padding: 4px;
        }
        QMenu::item {
            border-radius: 5px;
            min-height: 28px;
            padding: 2px 26px 2px 10px;
        }
        QMenu::item:selected {
            background: #eaf2ff;
            color: #245fe0;
        }
        QMenu::item:disabled { color: #aab3c2; }
        QMenu::separator {
            background: #e5eaf1;
            height: 1px;
            margin: 4px 6px;
        }
        QFrame#RecoveryEntryCard, QFrame#RecoveryFilePanel {
            background: #ffffff;
            border: 1px solid #e1e7ef;
            border-radius: 7px;
        }
        QFrame#RecoveryEntryCard:hover { border-color: #c8d8ee; }
        QFrame#RecoveryEntryIcon {
            background: #ffffff;
            border: 1px solid #cfd8e6;
            border-radius: 6px;
        }
        QPushButton#RecoveryEntryOpenButton {
            background: transparent;
            border: none;
            color: #172033;
            text-align: left;
            padding: 8px 0;
        }
        QPushButton#RecoveryEntryOpenButton:hover { color: #2f6df6; }
        QToolButton#RecoveryCopyButton, QToolButton#RecoveryBackButton,
        QToolButton#RecoverySelectButton {
            background: transparent;
            border: none;
            border-radius: 6px;
            color: #596579;
        }
        QToolButton#RecoveryCopyButton:hover, QToolButton#RecoveryBackButton:hover,
        QToolButton#RecoverySelectButton:hover {
            background: #eaf2ff;
            color: #2f6df6;
        }
        QFrame#RecoveryModePanel {
            background: #fffaf0;
            border: 1px solid #ead8ad;
            border-radius: 7px;
        }
        QLineEdit#RecoveryFilePath {
            background: #fbfdff;
            border: 1px solid #c7d8f1;
            border-radius: 6px;
            color: #293243;
            min-height: 32px;
            padding: 0 10px;
        }
        QPushButton#RecoveryStartButton {
            background: #2f6df6;
            border: none;
            border-radius: 6px;
            color: #ffffff;
            padding: 0 14px;
        }
        QPushButton#RecoveryStartButton:hover { background: #245fe0; }
        QPushButton#RecoveryStartButton:disabled {
            background: #e8edf5;
            color: #9aa5b5;
        }
        QPushButton#RecoveryCancelButton {
            background: #fff9f7;
            border: 1px solid #efc8c1;
            border-radius: 6px;
            color: #a24b42;
            padding: 0 12px;
        }
        QPushButton#RecoveryCancelButton:hover { background: #fff0ed; }
        QProgressBar#RecoveryProgressBar {
            background: #edf1f7;
            border: none;
            border-radius: 5px;
            color: #293243;
            min-height: 12px;
            text-align: center;
        }
        QProgressBar#RecoveryProgressBar::chunk {
            background: #3f9b69;
            border-radius: 5px;
        }
        QPlainTextEdit#RecoveryOutput {
            background: #111827;
            border: 1px solid #263244;
            border-radius: 7px;
            color: #dce6f4;
            padding: 10px;
            selection-background-color: #315b8c;
        }
    )") + QString::fromUtf8(R"(
        QFrame#FilesExplorer {
            background: #ffffff;
            border: 1px solid #dfe7f3;
            border-radius: 6px;
        }
        QWidget#FilesPage { background: #f2f6fb; }
        QWidget#FilesNavigationBar, QWidget#FilesActionBar, QWidget#FilesStatusBar {
            background: #ffffff;
            border: none;
        }
        QWidget#FilesNavigationBar, QWidget#FilesActionBar {
            border-bottom: 1px solid #e1e8f2;
        }
        QWidget#FilesStatusBar { border-top: 1px solid #e1e8f2; }
        QWidget#FilesContentArea, QWidget#FilesDeviceHome,
        QStackedWidget#FilesContentStack { background: #f8fafc; }
        QPushButton#FilesDriveCard {
            background: #ffffff;
            border: 1px solid #d8dee8;
            border-radius: 6px;
            padding: 0;
            text-align: left;
        }
        QPushButton#FilesDriveCard:hover {
            background: #fbfdff;
            border-color: #9db9dc;
        }
        QPushButton#FilesDriveCard:pressed {
            background: #f2f7fd;
            border-color: #76a3d8;
        }
        QLabel#FilesDriveIcon {
            background: #f4f6f9;
            border: 1px solid #d8dee8;
            border-radius: 6px;
        }
        QLabel#FilesDriveButton {
            background: transparent;
            border: none;
            color: #2d3138;
            font-size: 14px;
            font-weight: 600;
            padding: 0;
            text-align: left;
        }
        QProgressBar#FilesDriveProgress::chunk {
            background: #258bcb;
            border-radius: 2px;
        }
        QFrame#FilesDetailsPane {
            background: #f6f6f6;
            border: none;
            border-left: 1px solid #e0e4ea;
            border-radius: 0;
        }
        QLabel#FilesDetailsIcon {
            background: #f8fafc;
            border: none;
            border-bottom: 1px solid #e4e7eb;
        }
        QLabel#FilesDetailsHint {
            background: #fafafa;
            border: 1px solid #e0e0e0;
            border-radius: 5px;
            padding: 10px;
        }
        QWidget#FilesDetailRow {
            background: transparent;
            border: none;
        }
        QToolButton#FilesToolButton, QToolButton#FilesViewButton {
            background: transparent;
            border: none;
            border-radius: 6px;
            color: #596579;
        }
        QToolButton#FilesToolButton:hover, QToolButton#FilesViewButton:hover {
            background: #eaf2ff;
            color: #2f6df6;
        }
        QToolButton#FilesViewButton[active="true"] {
            background: #dceaff;
            color: #245fe0;
        }
        QToolButton#FilesDeleteButton {
            background: transparent;
            border: none;
            border-radius: 6px;
            color: #a24b42;
        }
        QToolButton#FilesDeleteButton:hover { background: #fff0ed; }
        QLineEdit#FilesAddressInput, QLineEdit#FilesSearchInput {
            background: #ffffff;
            border: 1px solid #c7d8f1;
            border-radius: 6px;
            color: #293243;
            min-height: 32px;
            padding: 0 10px;
        }
        QLineEdit#FilesAddressInput:focus, QLineEdit#FilesSearchInput:focus {
            border-color: #7da7ef;
        }
        QPushButton#FilesPrimaryButton {
            background: #eef3fa;
            border: 1px solid #d6e0ee;
            border-radius: 6px;
            color: #36506f;
            padding: 0 14px;
        }
        QPushButton#FilesPrimaryButton:hover {
            background: #e4edf9;
            border-color: #aec5e5;
        }
        QPushButton#FilesPrimaryButton[primary="true"] {
            background: #2f6df6;
            border-color: #2f6df6;
            color: #ffffff;
        }
        QPushButton#FilesPrimaryButton[primary="true"]:hover { background: #245fe0; }
        QPushButton#FilesPrimaryButton:disabled, QToolButton#FilesToolButton:disabled,
        QToolButton#FilesDeleteButton:disabled {
            background: #f2f4f7;
            border-color: #e4e8ee;
            color: #aab3c2;
        }
        QTableWidget#FilesTable {
            background: #ffffff;
            alternate-background-color: #fbfcfe;
            border: none;
            color: #293243;
            gridline-color: #edf1f6;
            outline: none;
        }
        QTableWidget#FilesTable::item {
            min-height: 34px;
            padding: 5px 8px;
            border-bottom: 1px solid #edf1f6;
        }
        QTableWidget#FilesTable::item:hover { background: #eef5ff; }
        QTableWidget#FilesTable::item:selected {
            background: #dceaff;
            color: #245fe0;
        }
        QHeaderView::section {
            background: #f1f5fb;
            border: none;
            border-bottom: 1px solid #dfe7f3;
            color: #596579;
            padding: 7px 9px;
            font-weight: 600;
        }
        QListWidget#FilesGrid {
            background: #ffffff;
            border: none;
            color: #293243;
            outline: none;
            padding: 8px;
        }
        QListWidget#FilesGrid::item {
            border: 1px solid transparent;
            border-radius: 7px;
            padding: 8px;
        }
        QListWidget#FilesGrid::item:hover {
            background: #eef5ff;
            border-color: #d6e3f5;
        }
        QListWidget#FilesGrid::item:selected {
            background: #dceaff;
            border-color: #9bbcf7;
            color: #245fe0;
        }
        QFrame#AppsCatalog, QFrame#AppsDetailPanel {
            background: #ffffff;
            border: 1px solid #dfe7f3;
            border-radius: 8px;
        }
        QWidget#AppsToolbar, QWidget#AppsFilters {
            background: #f8fafd;
            border: none;
            border-bottom: 1px solid #e1e8f2;
        }
        QLineEdit#AppsSearchInput {
            background: #ffffff;
            border: 1px solid #c7d8f1;
            border-radius: 6px;
            color: #293243;
            min-height: 34px;
            padding: 0 10px;
        }
        QLineEdit#AppsSearchInput:focus { border-color: #7da7ef; }
        QToolButton#AppsToolButton, QToolButton#AppsInstallButton {
            background: transparent;
            border: none;
            border-radius: 6px;
            color: #596579;
        }
        QToolButton#AppsToolButton:hover {
            background: #eaf2ff;
            color: #2f6df6;
        }
        QToolButton#AppsInstallButton {
            background: #2f6df6;
            color: #ffffff;
        }
        QToolButton#AppsInstallButton:hover { background: #245fe0; }
        QPushButton#AppsFilterButton {
            background: transparent;
            border: none;
            border-radius: 6px;
            color: #596579;
            padding: 0 8px;
        }
        QPushButton#AppsFilterButton:hover { background: #eef5ff; }
        QPushButton#AppsFilterButton[active="true"] {
            background: #dceaff;
            color: #245fe0;
        }
        QListWidget#AppsList {
            background: #ffffff;
            border: none;
            color: #293243;
            outline: none;
            padding: 10px;
        }
        QListWidget#AppsList::item {
            background: #fbfcfe;
            border: 1px solid #e2e8f1;
            border-radius: 7px;
            padding: 8px;
        }
        QListWidget#AppsList::item:hover {
            background: #eef5ff;
            border-color: #c8d8ee;
        }
        QListWidget#AppsList::item:selected {
            background: #dceaff;
            border-color: #9bbcf7;
            color: #245fe0;
        }
        QScrollArea#AppsDetailScroll {
            background: transparent;
            border: none;
        }
        QLabel#AppsHeroIcon {
            background: #eaf2ff;
            border: 1px solid #c7dafa;
            border-radius: 8px;
            color: #245fe0;
        }
        QPushButton#AppsActionButton {
            background: #eef3fa;
            border: 1px solid #d6e0ee;
            border-radius: 6px;
            color: #36506f;
            padding: 0 10px;
        }
        QPushButton#AppsActionButton:hover {
            background: #e4edf9;
            border-color: #aec5e5;
        }
        QPushButton#AppsActionButton[danger="true"] {
            background: #fff9f7;
            border-color: #efc8c1;
            color: #a24b42;
        }
        QPushButton#AppsActionButton[danger="true"]:hover { background: #fff0ed; }
        QPushButton#AppsActionButton:disabled, QToolButton#AppsToolButton:disabled,
        QToolButton#AppsInstallButton:disabled {
            background: #f2f4f7;
            border-color: #e4e8ee;
            color: #aab3c2;
        }
        QComboBox#AppsBackgroundMode {
            background: #ffffff;
            border: 1px solid #c7d8f1;
            border-radius: 6px;
            color: #293243;
            min-height: 32px;
            padding: 0 8px;
        }
        QFrame#AppsInfoPanel {
            background: #f8fafd;
            border: none;
            border-radius: 7px;
        }
        QWidget#AppsPermissions { background: transparent; }
        QWidget#AppsPermissionRow {
            background: #f8fafd;
            border: 1px solid #e5ebf3;
            border-radius: 6px;
        }
        QScrollBar:vertical {
            background: transparent;
            width: 8px;
            margin: 4px 0 4px 0;
        }
        QScrollBar::handle:vertical {
            background: #d6dfec;
            border-radius: 4px;
            min-height: 32px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )") + QString::fromUtf8(R"(
    )") + QString::fromUtf8(R"(
        QWidget#LayoutPage, QWidget#LayoutSessionBuilder, QWidget#LayoutSessionInspector {
            background: #f7f7f7;
        }
        QTabWidget#LayoutServerTabs::pane, QTabWidget#LayoutBuilderTabs::pane,
        QTabWidget#LayoutInspectorTabs::pane {
            border: none;
            background: transparent;
        }
        QTabWidget#LayoutServerTabs QTabBar::tab, QTabWidget#LayoutBuilderTabs QTabBar::tab,
        QTabWidget#LayoutInspectorTabs QTabBar::tab {
            background: transparent;
            border: none;
            border-bottom: 2px solid transparent;
            color: #172033;
            min-height: 34px;
            padding: 0 14px;
            font-size: 12px;
        }
        QTabWidget#LayoutServerTabs QTabBar::tab:selected,
        QTabWidget#LayoutBuilderTabs QTabBar::tab:selected,
        QTabWidget#LayoutInspectorTabs QTabBar::tab:selected {
            color: #1677ff;
            border-bottom-color: #1677ff;
            font-weight: 600;
        }
        QFrame#LayoutInputGroup, QFrame#LayoutAdvancedPanel, QFrame#LayoutCloudProviderForm,
        QFrame#LayoutCapabilityPanel, QFrame#LayoutJsonPanel, QFrame#LayoutSavedPanel,
        QFrame#LayoutInstruction, QFrame#LayoutSourceCard, QFrame#LayoutCommandsCard,
        QFrame#LayoutGestureCard, QFrame#LayoutRecorderCard, QFrame#LayoutSessionInfoCard,
        QFrame#LayoutSelectedElement, QFrame#LayoutScreenshotCard {
            background: #ffffff;
            border: 1px solid #d9d9d9;
            border-radius: 10px;
        }
        QFrame#LayoutInspectorHeader {
            background: #f8f8f8;
            border: none;
        }
        QFrame#LayoutCapabilityRow { background: transparent; border: none; }
        QLineEdit, QComboBox {
            background: #ffffff;
            border: 1px solid #d9d9d9;
            border-radius: 7px;
            color: #172033;
            min-height: 32px;
            padding: 0 9px;
        }
        QLineEdit:focus, QComboBox:focus { border-color: #1677ff; }
        QLineEdit:disabled { background: #f5f5f5; color: #a8adb5; }
        QCheckBox { spacing: 7px; color: #343a46; }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid #b8c0cb;
            border-radius: 4px;
            background: #ffffff;
        }
        QCheckBox::indicator:checked { background: #1677ff; border-color: #1677ff; }
        QToolButton#LayoutAdvancedButton {
            background: #fafafa;
            border: 1px solid #d9d9d9;
            border-radius: 9px;
            color: #172033;
            min-height: 42px;
            padding: 0 16px;
            text-align: left;
            font-size: 11px;
        }
        QPushButton#LayoutToolButton, QPushButton#LayoutCommandButton {
            background: #ffffff;
            border: 1px solid #d9d9d9;
            border-radius: 7px;
            color: #172033;
            padding: 0 12px;
        }
        QPushButton#LayoutToolButton:hover, QPushButton#LayoutCommandButton:hover {
            color: #1677ff;
            border-color: #91caff;
            background: #f0f7ff;
        }
        QPushButton#LayoutPrimaryButton, QPushButton#LayoutInspectorSelectedButton {
            background: #1677ff;
            border: 1px solid #1677ff;
            border-radius: 7px;
            color: #ffffff;
            padding: 0 14px;
        }
        QPushButton#LayoutPrimaryButton:hover, QPushButton#LayoutInspectorSelectedButton:hover {
            background: #0958d9;
            border-color: #0958d9;
        }
        QPushButton#LayoutDangerButton {
            background: #fff1f0;
            border: 1px solid #ffccc7;
            border-radius: 7px;
            color: #cf1322;
            padding: 0 14px;
        }
        QPlainTextEdit#LayoutJsonEditor, QPlainTextEdit {
            background: #ffffff;
            border: 1px solid #edf0f4;
            border-radius: 6px;
            color: #172033;
            padding: 10px;
            selection-background-color: #d9edff;
        }
        QScrollArea#LayoutCapabilityScroll { background: transparent; border: none; }
        QTableWidget#LayoutSavedTable, QTableWidget, QTreeWidget#LayoutSourceTree,
        QListWidget#LayoutSessionList {
            background: #ffffff;
            border: 1px solid #e4e7ec;
            border-radius: 7px;
            color: #172033;
            outline: none;
        }
        QTableWidget::item, QTreeWidget::item, QListWidget::item { padding: 6px 8px; }
        QTableWidget::item:selected, QTreeWidget::item:selected, QListWidget::item:selected {
            background: #e6f4ff;
            color: #0958d9;
        }
        QHeaderView::section {
            background: #fafafa;
            color: #596579;
            border: none;
            border-bottom: 1px solid #e4e7ec;
            padding: 8px;
            font-weight: 600;
        }
        QWidget#OverviewPage, QWidget#OverviewBody { background: #f5f4f8; }
        QScrollArea#OverviewScroll {
            background: transparent;
            border: none;
        }
        QFrame#OverviewHero, QFrame#OverviewMetricCard {
            background: #f0eef3;
            border: none;
            border-radius: 8px;
        }
        QFrame#OverviewFactCard {
            background: #faf9fb;
            border: 1px solid #d8d4df;
            border-radius: 8px;
        }
        QLabel#OverviewMetricIcon, QLabel#OverviewFactIcon {
            background: #e1d9ef;
            border: none;
            border-radius: 7px;
        }
        QProgressBar#OverviewMetricProgress, QProgressBar#FilesDriveProgress {
            background: #d9d6df;
            border: none;
            border-radius: 2px;
        }
        QProgressBar#OverviewMetricProgress::chunk {
            background: #5e438d;
            border-radius: 2px;
        }
        QPushButton#OverviewShizukuButton {
            background: #dfd6ef;
            border: none;
            border-radius: 7px;
            color: #352547;
            padding: 0 14px;
        }
        QPushButton#OverviewShizukuButton:hover { background: #d4c8e9; }
        QToolButton#OverviewPowerButton, QToolButton#OverviewRefreshButton {
            background: #e2daef;
            border: none;
            border-radius: 21px;
            color: #352547;
            font-size: 18px;
        }
        QToolButton#OverviewPowerButton:hover, QToolButton#OverviewRefreshButton:hover {
            background: #d5c9e8;
        }
        QFrame#OverviewPreviewPanel {
            background: #ffffff;
            border: 1px solid #dedbe4;
            border-radius: 8px;
        }
        QFrame#OverviewPhoneFrame {
            background: #14121b;
            border: 2px solid #383443;
            border-radius: 8px;
        }
        QLabel#OverviewScreenshot { background: #08070b; border-radius: 5px; }
        QPushButton#OverviewScreenshotButton {
            background: #f8f7fa;
            border: 1px solid #d3cedb;
            border-radius: 7px;
            color: #5e438d;
        }
        QPushButton#OverviewScreenshotButton:hover { background: #ebe6f2; }
        QPushButton#OverviewShizukuButton:disabled,
        QPushButton#OverviewScreenshotButton:disabled,
        QToolButton#OverviewPowerButton:disabled,
        QToolButton#OverviewRefreshButton:disabled {
            background: #e9e7eb;
            color: #aaa5b0;
        }
        QLabel#OverviewFactValue:hover { color: #5e438d; }
        QWidget#OverviewToolbar, QWidget#LogcatToolbar {
            background: #ffffff;
            border: none;
            border-bottom: 1px solid #dfe5ee;
        }
        QToolButton#OverviewToolButton, QToolButton#LogcatToolButton {
            background: transparent;
            border: none;
            border-radius: 6px;
            color: #435066;
        }
        QToolButton#OverviewToolButton:hover, QToolButton#LogcatToolButton:hover,
        QToolButton#LogcatToolButton[active="true"] {
            background: #eaf2ff;
            color: #2f6df6;
        }
        QToolButton#OverviewToolButton:disabled, QToolButton#LogcatToolButton:disabled {
            color: #b5bdc9;
            background: transparent;
        }
        QWidget#LogcatPage, QWidget#LogcatEmpty { background: #f6f8fb; }
        QLabel#LogcatStatus { color: #7b8798; padding-right: 4px; }
        QLabel#LogcatEmptyLabel { color: #8b96a8; }
        QComboBox#LogcatSelect, QLineEdit#LogcatFilterInput {
            background: #ffffff;
            border: 1px solid #cfd7e3;
            border-radius: 5px;
            color: #293243;
            min-height: 32px;
            padding: 0 9px;
        }
        QComboBox#LogcatSelect:focus, QLineEdit#LogcatFilterInput:focus {
            border-color: #7da7ef;
        }
        QTableView#LogcatTable {
            background: #fbfcfe;
            alternate-background-color: #f5f7fa;
            border: none;
            color: #303846;
            gridline-color: transparent;
            outline: none;
            selection-background-color: #dceaff;
            selection-color: #172033;
        }
        QTableView#LogcatTable::item {
            border: none;
            border-bottom: 1px solid #edf0f4;
            padding: 3px 6px;
        }
        QTableView#LogcatTable::item:selected { background: #dceaff; }
        QTableView#LogcatTable QHeaderView::section {
            background: #eef2f7;
            color: #596579;
            border: none;
            border-right: 1px solid #dde3ec;
            border-bottom: 1px solid #d6dde8;
            padding: 7px 8px;
            font-weight: 600;
        }
        QWidget#DisplayPage, QWidget#DisplayBody,
        QWidget#MirroringPage, QWidget#MirroringContent {
            background: #eef1f6;
        }
        QWidget#DisplayToolbar, QWidget#MirroringToolbar,
        QWidget#MirrorModeBar, QWidget#MirroringFooter {
            background: #ffffff;
            border: none;
            border-bottom: 1px solid #dfe5ee;
        }
        QLabel#DisplayStatus, QLabel#MirroringStatus { color: #7b8798; }
        QToolButton#DisplayToolButton, QToolButton#MirrorIconButton {
            background: transparent;
            border: none;
            border-radius: 6px;
            color: #435066;
        }
        QToolButton#DisplayToolButton:hover, QToolButton#MirrorIconButton:hover {
            background: #eaf2ff;
            color: #2f6df6;
        }
        QToolButton#DisplayToolButton:disabled, QToolButton#MirrorIconButton:disabled {
            color: #b5bdc9;
            background: transparent;
        }
        QFrame#DisplayInfoCard, QFrame#DisplayPanel,
        QFrame#MirrorCard {
            background: #ffffff;
            border: 1px solid #ccd5e2;
            border-radius: 8px;
        }
        QLabel#DisplayInfoIcon, QLabel#MirrorCardIcon {
            background: #eaf2ff;
            color: #2f6df6;
            border: none;
            border-radius: 6px;
        }
        QLabel#DisplayInfoLabel, QLabel#DisplayFieldLabel,
        QLabel#MirrorFieldLabel, QLabel#DisplayInputSuffix,
        QLabel#DisplaySliderLabel, QLabel#DisplayRatesEmpty,
        QLabel#MirroringFooterInfo {
            color: #657185;
        }
    )") + QString::fromUtf8(R"(
        QLabel#DisplayInfoValue, QLabel#DisplayPanelTitle,
        QLabel#MirrorCardTitle {
            color: #172033;
        }
        QFrame#DisplayInputFrame {
            background: #ffffff;
            border: 1px solid #cbd4e1;
            border-radius: 6px;
        }
        QFrame#DisplayInputFrame:focus-within { border-color: #6f9ced; }
        QLineEdit#DisplayInput {
            background: transparent;
            border: none;
            color: #172033;
            padding: 0;
        }
        QPushButton#DisplaySuggestionChip, QPushButton#DisplayRateChip {
            background: #f5f7fa;
            border: 1px solid #cbd4e1;
            border-radius: 6px;
            color: #435066;
            padding: 0 10px;
        }
        QPushButton#DisplaySuggestionChip:hover,
        QPushButton#DisplayRateChip:hover,
        QPushButton#DisplayRateChip[selected="true"] {
            background: #eaf2ff;
            border-color: #9bbcf4;
            color: #2f6df6;
        }
        QPushButton#DisplayResetButton {
            min-width: 70px;
            min-height: 38px;
            background: transparent;
            border: none;
            border-radius: 6px;
            color: #2f6df6;
        }
        QPushButton#DisplayResetButton:hover { background: #eef5ff; }
        QPushButton#DisplayApplyButton {
            min-width: 86px;
            min-height: 38px;
            background: #2f6df6;
            border: none;
            border-radius: 7px;
            color: #ffffff;
            padding: 0 16px;
        }
        QPushButton#DisplayApplyButton:hover { background: #245fe0; }
        QPushButton#DisplayApplyButton:disabled,
        QPushButton#DisplayResetButton:disabled {
            background: #e8edf5;
            color: #9aa5b5;
        }
        QFrame#DisplaySegment {
            background: #f7f9fc;
            border: 1px solid #cbd4e1;
            border-radius: 7px;
        }
        QToolButton#DisplaySegmentButton {
            background: transparent;
            border: none;
            border-right: 1px solid #d8dee8;
            color: #293243;
            padding: 0 8px;
        }
        QToolButton#DisplaySegmentButton:hover { background: #eef5ff; }
        QToolButton#DisplaySegmentButton:checked,
        QToolButton#DisplaySegmentButton[selected="true"] {
            background: #dceaff;
            color: #245fe0;
        }
        QSlider#DisplaySlider::groove:horizontal {
            height: 5px;
            background: #d7deea;
            border-radius: 2px;
        }
        QSlider#DisplaySlider::sub-page:horizontal {
            background: #2f6df6;
            border-radius: 2px;
        }
        QSlider#DisplaySlider::handle:horizontal {
            width: 16px;
            margin: -6px 0;
            background: #2f6df6;
            border: 2px solid #ffffff;
            border-radius: 8px;
        }
        QToolButton#MirrorModeButton {
            background: transparent;
            border: none;
            border-bottom: 2px solid transparent;
            color: #596579;
            padding: 8px 12px;
        }
        QToolButton#MirrorModeButton:hover {
            background: #f4f7fb;
            color: #2f6df6;
        }
        QToolButton#MirrorModeButton:checked {
            border-bottom-color: #2f6df6;
            color: #2f6df6;
        }
        QLineEdit#MirrorInput, QComboBox#MirrorSelect {
            min-height: 36px;
            background: #ffffff;
            border: 1px solid #cbd4e1;
            border-radius: 6px;
            color: #172033;
            padding: 0 10px;
        }
        QLineEdit#MirrorInput:focus, QComboBox#MirrorSelect:focus {
            border-color: #6f9ced;
        }
        QCheckBox#MirrorToggle {
            min-height: 40px;
            background: #f5f7fa;
            border: none;
            border-radius: 7px;
            color: #293243;
            padding: 0 12px;
            spacing: 9px;
        }
        QCheckBox#MirrorToggle:hover { background: #eef5ff; }
        QCheckBox#MirrorToggle::indicator {
            width: 17px;
            height: 17px;
            background: #ffffff;
            border: 1px solid #9ca8b9;
            border-radius: 8px;
        }
        QCheckBox#MirrorToggle::indicator:checked {
            background: #2f6df6;
            border-color: #2f6df6;
        }
        QWidget#MirroringFooter {
            border-top: 1px solid #d5dce7;
            border-bottom: none;
        }
        QPushButton#MirrorSecondaryButton {
            min-height: 40px;
            background: #ffffff;
            border: 1px solid #8e9bae;
            border-radius: 7px;
            color: #435066;
            padding: 0 14px;
        }
        QPushButton#MirrorSecondaryButton:hover {
            background: #eef5ff;
            border-color: #6f9ced;
            color: #2f6df6;
        }
        QPushButton#MirrorLaunchButton {
            min-height: 42px;
            background: #2f6df6;
            border: none;
            border-radius: 7px;
            color: #ffffff;
            padding: 0 18px;
        }
        QPushButton#MirrorLaunchButton:hover { background: #245fe0; }
        QPushButton#MirrorLaunchButton[running="true"] { background: #293243; }
        QPushButton#MirrorLaunchButton:disabled,
        QPushButton#MirrorSecondaryButton:disabled {
            background: #e8edf5;
            border-color: #d8dee8;
            color: #9aa5b5;
        }
        QWidget#OtherPage, QWidget#OtherContent,
        QWidget#ProcessPage { background: #f7f9fc; }
        QWidget#OtherToolbar, QWidget#ProcessToolbar {
            background: #ffffff;
            border-bottom: 1px solid #dce3ed;
        }
        QScrollArea#OtherScroll { background: transparent; border: none; }
        QFrame#OtherCommandList {
            background: #ffffff;
            border: 1px solid #dce3ed;
            border-radius: 6px;
        }
        QFrame#OtherCommandRow {
            background: #ffffff;
            border: none;
            border-bottom: 1px solid #e5eaf1;
        }
        QFrame#OtherCommandRow:hover { background: #f0f4fa; }
        QFrame#OtherCommandRow:disabled { background: #f7f8fa; }
        QLabel#OtherCommandIcon {
            background: #ffffff;
            border: 1px solid #cbd3df;
            border-radius: 6px;
        }
        QToolButton#OtherOpenButton,
        QToolButton#ProcessToolButton,
        QToolButton#ProcessStopButton {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 5px;
            color: #596579;
            font-size: 18px;
        }
        QToolButton#OtherOpenButton:hover,
        QToolButton#ProcessToolButton:hover {
            background: #e8f1ff;
            border-color: #c8dcfb;
            color: #2f6df6;
        }
        QToolButton#ProcessStopButton:hover {
            background: #fff0ed;
            border-color: #f0c4bb;
            color: #c84f42;
        }
        QToolButton#OtherOpenButton:disabled,
        QToolButton#ProcessToolButton:disabled,
        QToolButton#ProcessStopButton:disabled { color: #b5bdc9; }
        QPlainTextEdit#OtherOutput {
            background: #141922;
            border: 1px solid #2d3542;
            border-radius: 5px;
            color: #dce7f5;
            padding: 10px;
        }
    )") + QString::fromUtf8(R"(
        QStackedWidget#OtherContentStack, QWidget#OtherDetailCanvas,
        QWidget#OtherDetailContent, QScrollArea#OtherDetailScroll {
            background: #f5f5f7;
            border: none;
        }
        QWidget#OtherDetailHeader { background: transparent; border: none; }
        QLabel#OtherDetailIcon {
            background: #ffffff;
            border: 1px solid rgba(60, 60, 67, 32);
            border-radius: 8px;
        }
        QFrame#OtherSettingRow, QFrame#OtherSettingsGroup {
            background: rgba(255, 255, 255, 238);
            border: 1px solid rgba(60, 60, 67, 28);
            border-radius: 8px;
        }
        QLineEdit#OtherCommandInput, QComboBox#OtherServerCombo,
        QDoubleSpinBox#OtherNumericInput, QSpinBox#OtherNumericInput {
            min-height: 40px;
            background: #ffffff;
            border: 1px solid #d2d2d7;
            border-radius: 8px;
            color: #1d1d1f;
            padding: 0 12px;
            selection-background-color: #d6e8ff;
        }
        QLineEdit#OtherCommandInput:focus, QComboBox#OtherServerCombo:focus,
        QDoubleSpinBox#OtherNumericInput:focus, QSpinBox#OtherNumericInput:focus {
            border-color: #6ba5f7;
        }
        QPlainTextEdit#OtherResultOutput {
            background: rgba(255, 255, 255, 244);
            border: 1px solid rgba(60, 60, 67, 30);
            border-radius: 8px;
            color: #242426;
            padding: 14px;
            selection-background-color: #d6e8ff;
        }
        QPushButton#OtherActionButton {
            min-width: 96px;
            background: #ffffff;
            border: 1px solid #d2d2d7;
            border-radius: 8px;
            color: #1d1d1f;
            padding: 0 16px;
        }
        QPushButton#OtherActionButton:hover {
            background: #f0f5fc;
            border-color: #9abceb;
            color: #1768c5;
        }
        QPushButton#OtherActionButton[primary="true"] {
            background: #1677ff;
            border-color: #1677ff;
            color: #ffffff;
        }
        QPushButton#OtherActionButton[primary="true"]:hover {
            background: #0969da;
            border-color: #0969da;
        }
        QPushButton#OtherActionButton:pressed {
            background: #e5edf8;
        }
        QPushButton#OtherActionButton[primary="true"]:pressed {
            background: #0759ba;
        }
        QPushButton#OtherActionButton:disabled {
            background: #ececf0;
            border-color: #e0e0e5;
            color: #a1a1a6;
        }
        QToolButton#OtherBackButton {
            background: #ffffff;
            border: 1px solid #d2d2d7;
            border-radius: 8px;
            color: #1d1d1f;
            font-size: 18px;
        }
        QToolButton#OtherBackButton:hover {
            background: #eef5ff;
            border-color: #9abceb;
            color: #1677ff;
        }
        QLineEdit#ProcessFilterInput {
            min-height: 34px;
            background: #ffffff;
            border: 1px solid #cbd4e1;
            border-radius: 4px;
            padding: 0 10px;
            color: #293243;
        }
        QLineEdit#ProcessFilterInput:focus { border-color: #6f9ced; }
        QCheckBox#ProcessOnlyApps { color: #293243; spacing: 7px; }
        QFrame#ProcessSeparator { color: #d5dce7; }
        QTableView#ProcessTable {
            background: #f8fafc;
            alternate-background-color: #eef1f5;
            border: none;
            color: #333b49;
            gridline-color: #d6dce5;
            selection-background-color: #d9e8fb;
            selection-color: #172033;
        }
        QTableView#ProcessTable::item {
            border: none;
            border-right: 1px solid #d8dee7;
            padding: 3px 7px;
        }
        QTableView#ProcessTable::item:hover { background: #e7eef8; }
        QTableView#ProcessTable QHeaderView::section {
            min-height: 30px;
            background: #e8ebf0;
            border: none;
            border-right: 1px solid #cbd2dc;
            border-bottom: 1px solid #cbd2dc;
            color: #313a49;
            padding: 0 7px;
            text-align: left;
        }

        /* Apple-inspired material layer */
        QMainWindow, QFrame#Root, QStackedWidget#WorkspaceStack,
        QWidget#SettingsPage, QWidget#SettingsContent,
        QWidget#OverviewPage, QWidget#DisplayPage, QWidget#MirroringPage,
        QWidget#DeviceControlPage, QWidget#PackageManagerPage,
        QWidget#AppsPage, QWidget#FilesPage, QWidget#RecoveryPage,
        QWidget#PerformancePage, QWidget#LayoutPage, QWidget#LogcatPage,
        QWidget#OtherPage, QWidget#ProcessPage {
            background: #f5f5f7;
            color: #1d1d1f;
        }
        QFrame#Header {
            background: rgba(255, 255, 255, 232);
            border: none;
            border-bottom: 1px solid rgba(60, 60, 67, 30);
        }
        QFrame#Sidebar, QWidget#SidebarNavigation {
            background: rgba(250, 250, 252, 224);
            border: none;
        }
        QFrame#Sidebar {
            border-right: 1px solid rgba(60, 60, 67, 28);
        }
        QPushButton#WorkspaceNavButton {
            min-height: 42px;
            background: transparent;
            border: none;
            border-radius: 11px;
            color: #3a3a3c;
            padding: 0 14px;
        }
        QPushButton#WorkspaceNavButton:hover {
            background: rgba(118, 118, 128, 18);
            color: #1d1d1f;
        }
        QPushButton#WorkspaceNavButton[active="true"] {
            background: rgba(0, 113, 227, 24);
            color: #0066cc;
        }
        QPushButton#IconButton {
            min-width: 38px;
            min-height: 38px;
            background: rgba(255, 255, 255, 150);
            border: 1px solid rgba(60, 60, 67, 22);
            border-radius: 12px;
            color: #1d1d1f;
        }
        QPushButton#IconButton:hover,
        QPushButton#IconButton[active="true"] {
            background: rgba(0, 113, 227, 22);
            border-color: rgba(0, 113, 227, 48);
            color: #0071e3;
        }
        QPushButton#DeviceSelector {
            background: rgba(255, 255, 255, 190);
            border: 1px solid rgba(60, 60, 67, 24);
            border-radius: 12px;
            padding: 0;
            text-align: left;
        }
        QPushButton#DeviceSelector:hover {
            background: rgba(0, 113, 227, 12);
            border-color: rgba(0, 113, 227, 48);
        }
        QPushButton#DeviceSelector:pressed {
            background: rgba(0, 113, 227, 22);
            border-color: rgba(0, 113, 227, 68);
        }
        QFrame#Panel, QFrame#CommandCategory, QFrame#SettingsCard,
        QFrame#OverviewMetricCard, QFrame#OverviewInfoCard,
        QFrame#DisplayCard, QFrame#MirrorCard {
            background: rgba(255, 255, 255, 232);
            border: 1px solid rgba(60, 60, 67, 24);
            border-radius: 12px;
        }
        QLabel#SettingsTitle { color: #1d1d1f; }
        QLabel#SettingsSubtitle, QLabel#SettingsRowDetail { color: #6e6e73; }
        QLabel#SettingsRowTitle, QLabel#SettingsValue { color: #1d1d1f; }
        QScrollArea#SettingsScroll { background: transparent; border: none; }
        QFrame#SettingsSegmentedControl {
            min-width: 220px;
            min-height: 38px;
            background: rgba(118, 118, 128, 22);
            border: 1px solid rgba(60, 60, 67, 20);
            border-radius: 11px;
        }
        QPushButton#SettingsSegmentButton {
            min-width: 102px;
            min-height: 32px;
            background: transparent;
            border: none;
            border-radius: 8px;
            color: #6e6e73;
            padding: 0 12px;
        }
        QPushButton#SettingsSegmentButton:hover { color: #1d1d1f; }
        QPushButton#SettingsSegmentButton:checked {
            background: #ffffff;
            border: 1px solid rgba(60, 60, 67, 22);
            color: #1d1d1f;
        }
        QCheckBox#SettingsSwitch {
            min-height: 38px;
            color: #1d1d1f;
            spacing: 10px;
        }
        QCheckBox#SettingsSwitch::indicator {
            width: 42px;
            height: 24px;
            background: #d1d1d6;
            border: none;
            border-radius: 12px;
        }
        QCheckBox#SettingsSwitch::indicator:checked {
            background: #34c759;
            border: 5px solid #34c759;
        }
        QLineEdit, QComboBox, QSpinBox, QDoubleSpinBox {
            min-height: 36px;
            background: rgba(255, 255, 255, 236);
            border: 1px solid rgba(60, 60, 67, 42);
            border-radius: 10px;
            color: #1d1d1f;
            selection-background-color: #b9dcff;
        }
        QLineEdit:focus, QComboBox:focus, QSpinBox:focus, QDoubleSpinBox:focus {
            border: 2px solid #0071e3;
        }
        QPushButton#SendButton, QPushButton#MirrorLaunchButton,
        QPushButton#PackageInstallStartButton {
            background: #0071e3;
            border: none;
            border-radius: 11px;
            color: #ffffff;
        }
        QPushButton#SendButton:hover, QPushButton#MirrorLaunchButton:hover,
        QPushButton#PackageInstallStartButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                                        stop:0 #0071e3, stop:1 #0a84ff);
        }
        QProgressBar {
            min-height: 8px;
            max-height: 8px;
            background: rgba(118, 118, 128, 26);
            border: none;
            border-radius: 4px;
            color: transparent;
        }
        QProgressBar::chunk {
            background: #0071e3;
            border-radius: 4px;
        }
        QScrollBar:vertical {
            width: 10px;
            background: transparent;
            margin: 2px;
        }
        QScrollBar::handle:vertical {
            min-height: 32px;
            background: rgba(60, 60, 67, 70);
            border: 3px solid transparent;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical:hover { background: rgba(60, 60, 67, 105); }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            height: 0;
            background: transparent;
        }
        QMainWindow#DeviceCenterWindow, QWidget#DeviceCenterRoot,
        QWidget#DeviceCenterPreview {
            background: #f5f5f7;
            color: #1d1d1f;
        }
        QWidget#DeviceCenterToolbar {
            background: rgba(255, 255, 255, 238);
            border: none;
            border-bottom: 1px solid #d6d6da;
        }
        QFrame#DeviceCenterSeparator {
            color: #d2d2d7;
            background: #d2d2d7;
            max-width: 1px;
        }
        QLineEdit#DeviceCenterIpInput, QLineEdit#DeviceCenterPortInput,
        QLineEdit#DeviceCenterFilterInput {
            min-height: 32px;
            max-height: 32px;
            background: #ffffff;
            border: 1px solid #cfd2d8;
            border-radius: 5px;
            padding: 0 9px;
        }
        QLineEdit#DeviceCenterIpInput:focus, QLineEdit#DeviceCenterPortInput:focus,
        QLineEdit#DeviceCenterFilterInput:focus {
            border: 1px solid #4c91e8;
        }
        QPushButton#DeviceCenterCommandButton,
        QPushButton#DeviceCenterSecondaryButton,
        QPushButton#DeviceCenterPrimaryButton {
            min-height: 32px;
            background: #ffffff;
            border: 1px solid #c7cbd2;
            border-radius: 5px;
            color: #242426;
            padding: 0 14px;
        }
        QPushButton#DeviceCenterCommandButton:hover,
        QPushButton#DeviceCenterSecondaryButton:hover {
            background: #eef5ff;
            border-color: #8eb5e8;
            color: #075fad;
        }
        QPushButton#DeviceCenterPrimaryButton {
            background: #1677ff;
            border-color: #1677ff;
            color: #ffffff;
        }
        QPushButton#DeviceCenterCommandButton:disabled,
        QPushButton#DeviceCenterSecondaryButton:disabled,
        QPushButton#DeviceCenterPrimaryButton:disabled {
            background: #ececf0;
            border-color: #dedee3;
            color: #a1a1a6;
        }
        QToolButton#DeviceCenterToolButton {
            background: transparent;
            border: 1px solid transparent;
            border-radius: 5px;
            padding: 5px;
        }
        QToolButton#DeviceCenterToolButton:hover {
            background: #edf3fb;
            border-color: #c7d9ef;
        }
        QToolButton#DeviceCenterToolButton:pressed { background: #dce9f8; }
        QToolButton#DeviceCenterToolButton:disabled { opacity: 0.38; }
        QTableWidget#DeviceCenterTable {
            background: #f8f8f9;
            alternate-background-color: #f0f0f2;
            border: none;
            border-bottom: 1px solid #cfd2d8;
            color: #242426;
            gridline-color: #d6d8dd;
            selection-background-color: #dcecff;
            selection-color: #1d1d1f;
        }
        QTableWidget#DeviceCenterTable::item {
            border: none;
            border-right: 1px solid #d8dae0;
            padding: 3px 7px;
        }
        QTableWidget#DeviceCenterTable::item:hover { background: #e8f1fc; }
        QTableWidget#DeviceCenterTable QHeaderView::section {
            min-height: 30px;
            background: #e8e8eb;
            border: none;
            border-right: 1px solid #c9cbd1;
            border-bottom: 1px solid #c9cbd1;
            color: #26262a;
            padding: 0 7px;
            text-align: left;
        }
        QSplitter#DeviceCenterSplitter::handle {
            height: 5px;
            background: #d5d7dc;
        }
        QLabel#DeviceCenterPreviewLabel {
            background: transparent;
            border: none;
            color: #8e8e93;
            font-size: 18px;
        }
        QDialog#DeviceCenterPairDialog {
            background: #f5f5f7;
            color: #1d1d1f;
        }
        QToolTip {
            background: rgba(40, 40, 42, 242);
            border: 1px solid rgba(255, 255, 255, 24);
            border-radius: 8px;
            color: #ffffff;
            padding: 7px 9px;
        }
    )");
}

} // namespace ui
