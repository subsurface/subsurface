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
};
#endif
