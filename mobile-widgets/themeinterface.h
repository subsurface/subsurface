// SPDX-License-Identifier: GPL-2.0
#ifndef THEMEINTERFACE_H
#define THEMEINTERFACE_H
#include <QObject>
#include <QColor>
#include <QSettings>
#include <QQmlContext>

class ThemeInterface : public QObject {
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

public:
	static ThemeInterface *instance();
	double currentScale();
	void setInitialFontSize(double fontSize);

public slots:
	void set_currentTheme(const QString &theme);
	void set_currentScale(double);

signals:
	void backgroundColorChanged();
	void contrastAccentColorChanged();
	void darkerPrimaryColorChanged();
	void darkerPrimaryTextColorChanged();
	void drawerColorChanged();
	void lightDrawerColorChanged();
	void lightPrimaryColorChanged();
	void lightPrimaryTextColorChanged();
	void primaryColorChanged();
	void primaryTextColorChanged();
	void secondaryTextColorChanged();
	void textColorChanged();

	void headingPointSizeChanged();
	void regularPointSizeChanged();
	void smallPointSizeChanged();
	void titlePointSizeChanged();
	void currentScaleChanged();

	void currentThemeChanged();

private:
	ThemeInterface();
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

	double m_basePointSize;
	double m_headingPointSize;
	double m_regularPointSize;
	double m_smallPointSize;
	double m_titlePointSize;

	QString m_currentTheme;
};
#endif
