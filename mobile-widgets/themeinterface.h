// SPDX-License-Identifier: GPL-2.0
#ifndef THEMEINTERFACE_H
#define THEMEINTERFACE_H
#include <QObject>
#include <QColor>
#include <QSettings>

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

	// Support
	Q_PROPERTY(QString currentTheme MEMBER m_currentTheme WRITE set_currentTheme NOTIFY currentThemeChanged)

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

	void setup();

public slots:
	void set_currentTheme(const QString &theme);

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

	void currentThemeChanged(const QString &);

private:
	themeInterface() {}
	void update_theme();

	QColor m_backgroundColor;
	QColor m_contrastAccentColor;
	QColor m_darkerPrimaryColor;
	QColor m_darkerPrimaryTextColor;
	QColor m_drawerColor;
	QColor m_lightDrawerColor;
	QColor m_lightPrimaryColor;
	QColor m_lightPrimaryTextColor;
	QColor m_primaryColor;
	QColor m_primaryTextColor;
	QColor m_secondaryTextColor;
	QColor m_textColor;

	QString m_currentTheme;

	// Compatibility existing code
	const QColor m_blueBackgroundColor = "#eff0f1";
	const QColor m_blueContrastAccentColor = "#FF5722";
	const QColor m_blueDarkerPrimaryColor = "#303F9f";
	const QColor m_blueDarkerPrimaryTextColor = "#ECECEC";
	const QColor m_blueDrawerColor = "#FFFFFF";
	const QColor m_blueLightDrawerColor = "#FFFFFF";
	const QColor m_blueLightPrimaryColor = "#C5CAE9";
	const QColor m_blueLightPrimaryTextColor = "#212121";
	const QColor m_bluePrimaryColor = "#3F51B5";
	const QColor m_bluePrimaryTextColor = "#FFFFFF";
	const QColor m_blueSecondaryTextColor = "#757575";
	const QColor m_blueTextColor = "#212121";

	const QColor m_pinkBackgroundColor = "#eff0f1";
	const QColor m_pinkContrastAccentColor = "#FF5722";
	const QColor m_pinkDarkerPrimaryColor = "#C2185B";
	const QColor m_pinkDarkerPrimaryTextColor = "#ECECEC";
	const QColor m_pinkDrawerColor = "#FFFFFF";
	const QColor m_pinkLightDrawerColor = "#FFFFFF";
	const QColor m_pinkLightPrimaryColor = "#FFDDF4";
	const QColor m_pinkLightPrimaryTextColor = "#212121";
	const QColor m_pinkPrimaryColor = "#FF69B4";
	const QColor m_pinkPrimaryTextColor = "#212121";
	const QColor m_pinkSecondaryTextColor = "#757575";
	const QColor m_pinkTextColor = "#212121";

	const QColor m_darkBackgroundColor = "#303030";
	const QColor m_darkContrastAccentColor = "#FF5722";
	const QColor m_darkDarkerPrimaryColor = "#303F9f";
	const QColor m_darkDarkerPrimaryTextColor = "#ECECEC";
	const QColor m_darkDrawerColor = "#424242";
	const QColor m_darkLightDrawerColor = "#FFFFFF";
	const QColor m_darkLightPrimaryColor = "#C5CAE9";
	const QColor m_darkLightPrimaryTextColor = "#ECECEC";
	const QColor m_darkPrimaryColor = "#3F51B5";
	const QColor m_darkPrimaryTextColor = "#ECECEC";
	const QColor m_darkSecondaryTextColor = "#757575";
	const QColor m_darkTextColor = "#ECECEC";
};
#endif
