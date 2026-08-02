#include "ui/common/widget_helpers.h"

#include "ui/common/app_preferences.h"

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

} // namespace ui
