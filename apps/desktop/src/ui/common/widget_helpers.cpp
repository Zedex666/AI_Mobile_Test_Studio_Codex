#include "ui/common/widget_helpers.h"

#include <QFrame>

namespace ui {

QString text(const char *value)
{
    return QString::fromUtf8(value);
}

QFont appFont(int pointSize, QFont::Weight weight)
{
    QFont font(text("Microsoft YaHei UI"));
    font.setPointSize(pointSize);
    font.setWeight(weight);
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
