#include "ui/widgets/brand_badges.h"

#include "ui/common/widget_helpers.h"

#include <QPainter>

namespace ui {

LogoBadge::LogoBadge(QWidget *parent)
    : QWidget(parent),
      m_icon(imagePixmap(QStringLiteral("icons/app.png"), QSize(30, 30)))
{
    setFixedSize(34, 34);
}

void LogoBadge::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!m_icon.isNull()) {
        painter.drawPixmap(rect().adjusted(2, 2, -2, -2), m_icon);
        return;
    }

    QLinearGradient gradient(rect().topLeft(), rect().bottomRight());
    gradient.setColorAt(0.0, QColor("#8bb7ff"));
    gradient.setColorAt(1.0, QColor("#2f6df6"));
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect().adjusted(2, 2, -2, -2), 8, 8);

    painter.setPen(QPen(QColor("#ffffff"), 2.0));
    painter.drawArc(QRectF(9, 9, 16, 16), 25 * 16, 310 * 16);
    painter.drawEllipse(QPointF(13, 13), 2.2, 2.2);
    painter.drawEllipse(QPointF(22, 14), 2.2, 2.2);
    painter.drawEllipse(QPointF(17, 23), 2.2, 2.2);
    painter.drawLine(QPointF(14, 18), QPointF(20, 18));
}

AvatarBadge::AvatarBadge(QWidget *parent)
    : QWidget(parent)
{
    setFixedSize(42, 42);
}

void AvatarBadge::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor("#eef3fb"));
    painter.setPen(QPen(QColor("#d6e0ef"), 1));
    painter.drawEllipse(rect().adjusted(1, 1, -1, -1));

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#1f2937"));
    painter.drawEllipse(QPointF(width() / 2.0, 15), 7, 7);
    painter.setBrush(QColor("#2d5fec"));
    painter.drawRoundedRect(QRectF(10, 25, 22, 12), 6, 6);
    painter.setBrush(QColor("#f4c7a1"));
    painter.drawEllipse(QPointF(width() / 2.0, 16), 5, 6);
}

} // namespace ui
