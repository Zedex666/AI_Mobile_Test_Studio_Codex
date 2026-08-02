#ifndef AI_MOBILE_TEST_STUDIO_SETTINGS_PAGE_H
#define AI_MOBILE_TEST_STUDIO_SETTINGS_PAGE_H

#include <QWidget>

class QCheckBox;
class QLabel;
class QPushButton;

class SettingsPage : public QWidget
{
public:
    explicit SettingsPage(QWidget *parent = nullptr);

    void refreshPreferences();

private:
    QPushButton *m_chineseButton = nullptr;
    QPushButton *m_englishButton = nullptr;
    QCheckBox *m_motionCheckBox = nullptr;
    QLabel *m_fontValueLabel = nullptr;
};

#endif // AI_MOBILE_TEST_STUDIO_SETTINGS_PAGE_H
