#ifndef AI_MOBILE_TEST_STUDIO_WIDGET_HELPERS_H
#define AI_MOBILE_TEST_STUDIO_WIDGET_HELPERS_H

#include <QFont>
#include <QFrame>
#include <QIcon>
#include <QPixmap>
#include <QSize>
#include <QString>

namespace ui {

QString text(const char *value);
QFont appFont(int pointSize, QFont::Weight weight = QFont::Normal);
QFrame *makePanel(const QString &objectName = QString());
QString imageResourcePath(const QString &relativePath);
QIcon imageIcon(const QString &relativePath);
QPixmap imagePixmap(const QString &relativePath, const QSize &size);

} // namespace ui

#endif // AI_MOBILE_TEST_STUDIO_WIDGET_HELPERS_H
