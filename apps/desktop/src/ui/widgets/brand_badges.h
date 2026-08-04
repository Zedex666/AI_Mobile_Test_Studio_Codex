#ifndef AI_MOBILE_TEST_STUDIO_BRAND_BADGES_H
#define AI_MOBILE_TEST_STUDIO_BRAND_BADGES_H

#include <QPixmap>
#include <QWidget>

namespace ui {

class LogoBadge : public QWidget
{
public:
    explicit LogoBadge(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap m_icon;
};

class AvatarBadge : public QWidget
{
public:
    explicit AvatarBadge(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};

} // namespace ui

#endif // AI_MOBILE_TEST_STUDIO_BRAND_BADGES_H
