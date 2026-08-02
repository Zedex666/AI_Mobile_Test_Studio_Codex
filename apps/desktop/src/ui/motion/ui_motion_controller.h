#ifndef AI_MOBILE_TEST_STUDIO_UI_MOTION_CONTROLLER_H
#define AI_MOBILE_TEST_STUDIO_UI_MOTION_CONTROLLER_H

#include <QGraphicsEffect>
#include <QHash>
#include <QObject>
#include <QPointer>

class QAbstractButton;
class QVariantAnimation;
class QWidget;

namespace ui {

class ButtonMotionEffect : public QGraphicsEffect
{
    Q_OBJECT
    Q_PROPERTY(qreal scale READ scale WRITE setScale)
    Q_PROPERTY(qreal opacity READ opacity WRITE setOpacity)

public:
    explicit ButtonMotionEffect(QObject *parent = nullptr);

    qreal scale() const;
    void setScale(qreal scale);

    qreal opacity() const;
    void setOpacity(qreal opacity);

protected:
    void draw(QPainter *painter) override;
    QRectF boundingRectFor(const QRectF &sourceRect) const override;

private:
    qreal m_scale = 1.0;
    qreal m_opacity = 1.0;
};

class UiMotionController : public QObject
{
    Q_OBJECT

public:
    explicit UiMotionController(QObject *parent = nullptr);

    void attach(QWidget *root);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void animate(QAbstractButton *button, qreal scale, qreal opacity, int duration);
    void resetEffects();

    QHash<QAbstractButton *, QPointer<QVariantAnimation>> m_animations;
    QHash<QAbstractButton *, QPointer<ButtonMotionEffect>> m_effects;
};

} // namespace ui

#endif // AI_MOBILE_TEST_STUDIO_UI_MOTION_CONTROLLER_H
