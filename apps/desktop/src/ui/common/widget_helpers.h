#ifndef AI_MOBILE_TEST_STUDIO_WIDGET_HELPERS_H
#define AI_MOBILE_TEST_STUDIO_WIDGET_HELPERS_H

#include <QFont>
#include <QFrame>
#include <QString>

namespace ui {

QString text(const char *value);
QFont appFont(int pointSize, QFont::Weight weight = QFont::Normal);
QFrame *makePanel(const QString &objectName = QString());

} // namespace ui

#endif // AI_MOBILE_TEST_STUDIO_WIDGET_HELPERS_H
