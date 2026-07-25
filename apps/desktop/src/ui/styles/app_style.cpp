#include "ui/styles/app_style.h"

namespace ui {

QString appStyleSheet()
{
    return QString::fromUtf8(R"(
        QWidget {
            font-family: "Microsoft YaHei UI", "Segoe UI", sans-serif;
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
        QFrame#Toolbar {
            background: #ffffff;
            border-bottom: 1px solid #dfe7f3;
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
        QFrame#DeviceSelector {
            background: #ffffff;
            border: 1px solid #d9e4f4;
            border-radius: 8px;
        }
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
        QFrame#SideStatus {
            background: #f6f9ff;
            border: 1px solid #dfe7f3;
            border-radius: 8px;
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
        QPushButton#MirrorButton {
            background: #2f6df6;
            border: none;
            border-radius: 8px;
            color: #ffffff;
            padding: 0 12px;
        }
        QPushButton#MirrorButton:hover { background: #245fe0; }
        QPushButton#MirrorButton:disabled {
            background: #e8edf5;
            color: #9aa5b5;
        }
        QPushButton#MirrorButton[running="true"] {
            background: #293243;
            color: #ffffff;
        }
        QPushButton#MirrorButton[running="true"]:hover { background: #1d2533; }
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
    )") + QString::fromUtf8(R"(
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
            font-family: "Cascadia Mono", "Consolas", monospace;
            padding: 10px;
            selection-background-color: #315b8c;
        }
        QFrame#FilesExplorer {
            background: #ffffff;
            border: 1px solid #dfe7f3;
            border-radius: 8px;
        }
        QWidget#FilesNavigationBar, QWidget#FilesActionBar, QWidget#FilesStatusBar {
            background: #f8fafd;
            border: none;
        }
        QWidget#FilesNavigationBar, QWidget#FilesActionBar {
            border-bottom: 1px solid #e1e8f2;
        }
        QWidget#FilesStatusBar { border-top: 1px solid #e1e8f2; }
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
    )");
}

} // namespace ui
