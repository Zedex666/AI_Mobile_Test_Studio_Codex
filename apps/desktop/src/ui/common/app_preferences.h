#ifndef AI_MOBILE_TEST_STUDIO_APP_PREFERENCES_H
#define AI_MOBILE_TEST_STUDIO_APP_PREFERENCES_H

#include <QObject>
#include <QString>

class QWidget;

namespace ui {

enum class AppLanguage {
    Chinese,
    English
};

class AppPreferences : public QObject
{
    Q_OBJECT

public:
    static AppPreferences &instance();

    void initialize();

    AppLanguage language() const;
    void setLanguage(AppLanguage language);

    bool motionEnabled() const;
    void setMotionEnabled(bool enabled);

    QString fontFamily() const;
    QString translate(const char *source) const;
    QString translateExistingText(const QString &value) const;
    void applyFont(QWidget *root = nullptr) const;
    void retranslate(QWidget *root) const;

signals:
    void languageChanged(ui::AppLanguage language);
    void motionEnabledChanged(bool enabled);

private:
    explicit AppPreferences(QObject *parent = nullptr);

    void loadFonts();
    QString translatedValue(const QString &source) const;

    bool m_initialized = false;
    AppLanguage m_language = AppLanguage::Chinese;
    bool m_motionEnabled = true;
    QString m_chineseFontFamily;
    QString m_englishFontFamily;
};

} // namespace ui

Q_DECLARE_METATYPE(ui::AppLanguage)

#endif // AI_MOBILE_TEST_STUDIO_APP_PREFERENCES_H
