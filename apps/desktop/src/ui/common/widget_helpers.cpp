#include "ui/common/widget_helpers.h"

#include "ui/common/app_preferences.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFrame>

namespace ui {

QString text(const char *value)
{
    return AppPreferences::instance().translate(value);
}

QFont appFont(int pointSize, QFont::Weight weight)
{
    QFont font(AppPreferences::instance().fontFamily());
    font.setPointSize(pointSize);
    font.setWeight(weight);
    font.setStyleHint(AppPreferences::instance().language() == AppLanguage::English
                          ? QFont::Monospace
                          : QFont::SansSerif);
    return font;
}

QFrame *makePanel(const QString &objectName)
{
    auto *panel = new QFrame;
    panel->setObjectName(objectName.isEmpty() ? "Panel" : objectName);
    panel->setFrameShape(QFrame::NoFrame);
    return panel;
}

QString imageResourcePath(const QString &relativePath)
{
    return QDir(QCoreApplication::applicationDirPath())
        .filePath(QStringLiteral("runtime/images/") + relativePath);
}

QIcon imageIcon(const QString &relativePath)
{
    const QString path = imageResourcePath(relativePath);
    return QFileInfo::exists(path) ? QIcon(path) : QIcon();
}

QPixmap imagePixmap(const QString &relativePath, const QSize &size)
{
    return imageIcon(relativePath).pixmap(size);
}

} // namespace ui
