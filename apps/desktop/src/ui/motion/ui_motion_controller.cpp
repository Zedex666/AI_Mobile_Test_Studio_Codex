#include "ui/motion/ui_motion_controller.h"

#include "ui/common/app_preferences.h"

#include <QAbstractButton>
#include <QEvent>
#include <QPainter>
#include <QVariantAnimation>
#include <QWidget>

namespace ui {

ButtonMotionEffect::ButtonMotionEffect(QObject *parent)
    : QGraphicsEffect(parent)
{
}

qreal ButtonMotionEffect::scale() const
{
    return m_scale;
}

void ButtonMotionEffect::setScale(qreal scale)
{
    if (qFuzzyCompare(m_scale, scale)) {
        return;
    }
    m_scale = scale;
    updateBoundingRect();
    update();
}

qreal ButtonMotionEffect::opacity() const
{
    return m_opacity;
}

void ButtonMotionEffect::setOpacity(qreal opacity)
{
    if (qFuzzyCompare(m_opacity, opacity)) {
        return;
    }
    m_opacity = opacity;
    update();
}

void ButtonMotionEffect::draw(QPainter *painter)
{
    QPoint offset;
    const QPixmap source = sourcePixmap(Qt::LogicalCoordinates,
                                        &offset,
                                        QGraphicsEffect::PadToEffectiveBoundingRect);
    if (source.isNull()) {
        return;
    }

    const qreal deviceRatio = source.devicePixelRatio();
    const QSizeF logicalSize = QSizeF(source.size()) / deviceRatio;
    const QRectF sourceBounds(QPointF(offset), logicalSize);

    painter->save();
    painter->setOpacity(m_opacity);
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->translate(sourceBounds.center());
    painter->scale(m_scale, m_scale);
    painter->translate(-sourceBounds.center());
    painter->drawPixmap(offset, source);
    painter->restore();
}

QRectF ButtonMotionEffect::boundingRectFor(const QRectF &sourceRect) const
{
    const qreal margin = qMax<qreal>(4.0, qMax(sourceRect.width(), sourceRect.height()) * 0.02);
    return sourceRect.adjusted(-margin, -margin, margin, margin);
}

UiMotionController::UiMotionController(QObject *parent)
    : QObject(parent)
{
    connect(&AppPreferences::instance(),
            &AppPreferences::motionEnabledChanged,
            this,
            [this](bool enabled) {
                if (!enabled) {
                    resetEffects();
                }
            });
}

void UiMotionController::attach(QWidget *root)
{
    if (root == nullptr) {
        return;
    }
    const QList<QAbstractButton *> buttons = root->findChildren<QAbstractButton *>();
    for (QAbstractButton *button : buttons) {
        button->installEventFilter(this);
        connect(button, &QObject::destroyed, this, [this, button] {
            m_animations.remove(button);
            m_effects.remove(button);
        });
    }
}

bool UiMotionController::eventFilter(QObject *watched, QEvent *event)
{
    auto *button = qobject_cast<QAbstractButton *>(watched);
    if (button == nullptr || !button->isEnabled()
        || !AppPreferences::instance().motionEnabled()) {
        return QObject::eventFilter(watched, event);
    }

    const bool isWorkspaceNavigation = button->objectName() == QStringLiteral("WorkspaceNavButton");
    switch (event->type()) {
    case QEvent::Enter:
        if (!isWorkspaceNavigation) {
            animate(button, 1.008, 1.0, 140);
        }
        break;
    case QEvent::Leave:
        if (m_effects.contains(button)) {
            animate(button, 1.0, 1.0, 150);
        }
        break;
    case QEvent::MouseButtonPress:
        animate(button, isWorkspaceNavigation ? 1.0 : 0.985, 0.96, 90);
        break;
    case QEvent::MouseButtonRelease:
        animate(button,
                !isWorkspaceNavigation && button->underMouse() ? 1.008 : 1.0,
                1.0,
                150);
        break;
    default:
        break;
    }
    return QObject::eventFilter(watched, event);
}

void UiMotionController::animate(QAbstractButton *button,
                                 qreal targetScale,
                                 qreal targetOpacity,
                                 int duration)
{
    if (button == nullptr) {
        return;
    }

    if (QVariantAnimation *running = m_animations.value(button)) {
        running->stop();
        running->deleteLater();
    }

    ButtonMotionEffect *effect = m_effects.value(button);
    if (effect == nullptr) {
        if (button->graphicsEffect() != nullptr) {
            return;
        }
        effect = new ButtonMotionEffect(button);
        button->setGraphicsEffect(effect);
        m_effects.insert(button, effect);
    }

    auto *animation = new QVariantAnimation(button);
    animation->setDuration(duration);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->setStartValue(QPointF(effect->scale(), effect->opacity()));
    animation->setEndValue(QPointF(targetScale, targetOpacity));
    connect(animation,
            &QVariantAnimation::valueChanged,
            effect,
            [effect](const QVariant &value) {
                const QPointF state = value.toPointF();
                effect->setScale(state.x());
                effect->setOpacity(state.y());
            });
    connect(animation,
            &QVariantAnimation::finished,
            this,
            [this, button, effect, targetScale, targetOpacity] {
                m_animations.remove(button);
                if (qFuzzyCompare(targetScale, 1.0)
                    && qFuzzyCompare(targetOpacity, 1.0)) {
                    m_effects.remove(button);
                    button->setGraphicsEffect(nullptr);
                }
            });
    m_animations.insert(button, animation);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void UiMotionController::resetEffects()
{
    const QList<QAbstractButton *> buttons = m_effects.keys();
    for (QAbstractButton *button : buttons) {
        if (button == nullptr) {
            continue;
        }
        if (QVariantAnimation *animation = m_animations.value(button)) {
            animation->stop();
        }
        button->setGraphicsEffect(nullptr);
    }
    m_animations.clear();
    m_effects.clear();
}

} // namespace ui
