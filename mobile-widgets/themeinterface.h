// SPDX-License-Identifier: GPL-2.0
#ifndef THEMEINTERFACE_H
#define THEMEINTERFACE_H
#include <QObject>
#include <QColor>
#include <QSettings>
#include <QQmlContext>

class themeInterface : public QObject {
	Q_OBJECT

	// Color themes
	Q_PROPERTY(QColor backgroundColor MEMBER m_backgroundColor NOTIFY backgroundColorChanged)
	Q_PROPERTY(QColor contrastAccentColor MEMBER m_contrastAccentColor NOTIFY contrastAccentColorChanged)
	Q_PROPERTY(QColor darkerPrimaryColor MEMBER m_darkerPrimaryColor NOTIFY darkerPrimaryColorChanged)
	Q_PROPERTY(QColor darkerPrimaryTextColor MEMBER m_darkerPrimaryTextColor NOTIFY darkerPrimaryTextColorChanged)
	Q_PROPERTY(QColor drawerColor MEMBER m_drawerColor NOTIFY drawerColorChanged)
	Q_PROPERTY(QColor lightDrawerColor MEMBER m_lightDrawerColor NOTIFY lightDrawerColorChanged)
	Q_PROPERTY(QColor lightPrimaryColor MEMBER m_lightPrimaryColor NOTIFY lightPrimaryColorChanged)
	Q_PROPERTY(QColor lightPrimaryTextColor MEMBER m_lightPrimaryTextColor NOTIFY lightPrimaryTextColorChanged)
	Q_PROPERTY(QColor primaryColor MEMBER m_primaryColor NOTIFY primaryColorChanged)
	Q_PROPERTY(QColor primaryTextColor MEMBER m_primaryTextColor NOTIFY primaryTextColorChanged)
	Q_PROPERTY(QColor secondaryTextColor MEMBER m_secondaryTextColor NOTIFY secondaryTextColorChanged)
	Q_PROPERTY(QColor textColor MEMBER m_textColor NOTIFY textColorChanged)

	// Font
	Q_PROPERTY(double basePointSize MEMBER m_basePointSize CONSTANT)
	Q_PROPERTY(double headingPointSize MEMBER m_headingPointSize NOTIFY headingPointSizeChanged)
	Q_PROPERTY(double regularPointSize MEMBER m_regularPointSize NOTIFY regularPointSizeChanged)
	Q_PROPERTY(double smallPointSize MEMBER m_smallPointSize NOTIFY smallPointSizeChanged)
	Q_PROPERTY(double titlePointSize MEMBER m_titlePointSize NOTIFY titlePointSizeChanged)
	Q_PROPERTY(double currentScale READ currentScale WRITE set_currentScale NOTIFY currentScaleChanged)

	// Support
	Q_PROPERTY(QString currentTheme MEMBER m_currentTheme WRITE set_currentTheme NOTIFY currentThemeChanged)
	Q_PROPERTY(QString iconStyle MEMBER m_iconStyle NOTIFY iconStyleChanged)

	// Compatibility existing code
	Q_PROPERTY(QColor blueBackgroundColor MEMBER m_blueBackgroundColor CONSTANT)
	Q_PROPERTY(QColor blueContrastAccentColor MEMBER m_blueTextColor CONSTANT)
	Q_PROPERTY(QColor blueDarkerPrimaryColor MEMBER m_blueDarkerPrimaryColor CONSTANT)
	Q_PROPERTY(QColor blueDarkerPrimaryTextColor MEMBER m_blueDarkerPrimaryTextColor CONSTANT)
	Q_PROPERTY(QColor blueDrawerColor MEMBER m_blueDrawerColor CONSTANT)
	Q_PROPERTY(QColor blueLightDrawerColor MEMBER m_blueLightDrawerColor CONSTANT)
	Q_PROPERTY(QColor blueLightPrimaryColor MEMBER m_blueLightPrimaryColor CONSTANT)
	Q_PROPERTY(QColor blueLightPrimaryTextColor MEMBER m_blueLightPrimaryTextColor CONSTANT)
	Q_PROPERTY(QColor bluePrimaryColor MEMBER m_bluePrimaryColor CONSTANT)
	Q_PROPERTY(QColor bluePrimaryTextColor MEMBER m_bluePrimaryTextColor CONSTANT)
	Q_PROPERTY(QColor blueSecondaryTextColor MEMBER m_blueSecondaryTextColor CONSTANT)
	Q_PROPERTY(QColor blueTextColor MEMBER m_blueTextColor CONSTANT)

	Q_PROPERTY(QColor pinkBackgroundColor MEMBER m_pinkBackgroundColor CONSTANT)
	Q_PROPERTY(QColor pinkContrastAccentColor MEMBER m_pinkContrastAccentColor CONSTANT)
	Q_PROPERTY(QColor pinkDarkerPrimaryColor MEMBER m_blueDarkerPrimaryColor CONSTANT)
	Q_PROPERTY(QColor pinkDarkerPrimaryTextColor MEMBER m_blueDarkerPrimaryTextColor CONSTANT)
	Q_PROPERTY(QColor pinkDrawerColor MEMBER m_pinkDrawerColor CONSTANT)
	Q_PROPERTY(QColor pinkLightDrawerColor MEMBER m_pinkLightDrawerColor CONSTANT)
	Q_PROPERTY(QColor pinkLightPrimaryColor MEMBER m_blueLightPrimaryColor CONSTANT)
	Q_PROPERTY(QColor pinkLightPrimaryTextColor MEMBER m_blueLightPrimaryTextColor CONSTANT)
	Q_PROPERTY(QColor pinkPrimaryColor MEMBER m_pinkPrimaryColor CONSTANT)
	Q_PROPERTY(QColor pinkPrimaryTextColor MEMBER m_pinkPrimaryTextColor CONSTANT)
	Q_PROPERTY(QColor pinkSecondaryTextColor MEMBER m_blueSecondaryTextColor CONSTANT)
	Q_PROPERTY(QColor pinkTextColor MEMBER m_pinkTextColor CONSTANT)

	Q_PROPERTY(QColor darkBackgroundColor MEMBER m_darkBackgroundColor CONSTANT)
	Q_PROPERTY(QColor darkContrastAccentColor MEMBER m_darkContrastAccentColor CONSTANT)
	Q_PROPERTY(QColor darkDarkerPrimaryColor MEMBER m_blueDarkerPrimaryColor CONSTANT)
	Q_PROPERTY(QColor darkDarkerPrimaryTextColor MEMBER m_blueDarkerPrimaryTextColor CONSTANT)
	Q_PROPERTY(QColor darkDrawerColor MEMBER m_drawerColor CONSTANT)
	Q_PROPERTY(QColor darkLightDrawerColor MEMBER m_darkLightDrawerColor CONSTANT)
	Q_PROPERTY(QColor darkLightPrimaryColor MEMBER m_blueLightPrimaryColor CONSTANT)
	Q_PROPERTY(QColor darkLightPrimaryTextColor MEMBER m_blueLightPrimaryTextColor CONSTANT)
	Q_PROPERTY(QColor darkPrimaryColor MEMBER m_darkPrimaryColor CONSTANT)
	Q_PROPERTY(QColor darkPrimaryTextColor MEMBER m_darkPrimaryTextColor CONSTANT)
	Q_PROPERTY(QColor darkSecondaryTextColor MEMBER m_blueSecondaryTextColor CONSTANT)
	Q_PROPERTY(QColor darkTextColor MEMBER m_darkTextColor CONSTANT)

public:
	static themeInterface *instance();

	static void setup(QQmlContext *ct);

	static double currentScale();

public slots:
	static void set_currentTheme(const QString &theme);

	static void set_currentScale(double);

signals:
	void backgroundColorChanged(QColor);
	void contrastAccentColorChanged(QColor);
	void darkerPrimaryColorChanged(QColor);
	void darkerPrimaryTextColorChanged(QColor);
	void drawerColorChanged(QColor);
	void lightDrawerColorChanged(QColor);
	void lightPrimaryColorChanged(QColor);
	void lightPrimaryTextColorChanged(QColor);
	void primaryColorChanged(QColor);
	void primaryTextColorChanged(QColor);
	void secondaryTextColorChanged(QColor);
	void textColorChanged(QColor);

	void headingPointSizeChanged(double);
	void regularPointSizeChanged(double);
	void smallPointSizeChanged(double);
	void titlePointSizeChanged(double);
	void currentScaleChanged(double);

	void currentThemeChanged(const QString &);
	void iconStyleChanged(const QString &);

private:
	themeInterface() {}
	static void update_theme();

	static QColor m_backgroundColor;
	static QColor m_contrastAccentColor;
	static QColor m_darkerPrimaryColor;
	static QColor m_darkerPrimaryTextColor;
	static QColor m_drawerColor;
	static QColor m_lightDrawerColor;
	static QColor m_lightPrimaryColor;
	static QColor m_lightPrimaryTextColor;
	static QColor m_primaryColor;
	static QColor m_primaryTextColor;
	static QColor m_secondaryTextColor;
	static QColor m_textColor;

	static double m_basePointSize;
	static double m_headingPointSize;
	static double m_regularPointSize;
	static double m_smallPointSize;
	static double m_titlePointSize;

	static QString m_currentTheme;
	static QString m_iconStyle;

	// Compatibility existing code
	static const QColor m_blueBackgroundColor;
	static const QColor m_blueContrastAccentColor;
	static const QColor m_blueDarkerPrimaryColor;
	static const QColor m_blueDarkerPrimaryTextColor;
	static const QColor m_blueDrawerColor;
	static const QColor m_blueLightDrawerColor;
	static const QColor m_blueLightPrimaryColor;
	static const QColor m_blueLightPrimaryTextColor;
	static const QColor m_bluePrimaryColor;
	static const QColor m_bluePrimaryTextColor;
	static const QColor m_blueSecondaryTextColor;
	static const QColor m_blueTextColor;

	static const QColor m_pinkBackgroundColor;
	static const QColor m_pinkContrastAccentColor;
	static const QColor m_pinkDarkerPrimaryColor;
	static const QColor m_pinkDarkerPrimaryTextColor;
	static const QColor m_pinkDrawerColor;
	static const QColor m_pinkLightDrawerColor;
	static const QColor m_pinkLightPrimaryColor;
	static const QColor m_pinkLightPrimaryTextColor;
	static const QColor m_pinkPrimaryColor;
	static const QColor m_pinkPrimaryTextColor;
	static const QColor m_pinkSecondaryTextColor;
	static const QColor m_pinkTextColor;

	static const QColor m_darkBackgroundColor;
	static const QColor m_darkContrastAccentColor;
	static const QColor m_darkDarkerPrimaryColor;
	static const QColor m_darkDarkerPrimaryTextColor;
	static const QColor m_darkDrawerColor;
	static const QColor m_darkLightDrawerColor;
	static const QColor m_darkLightPrimaryColor;
	static const QColor m_darkLightPrimaryTextColor;
	static const QColor m_darkPrimaryColor;
	static const QColor m_darkPrimaryTextColor;
	static const QColor m_darkSecondaryTextColor;
	static const QColor m_darkTextColor;
};
#endif
