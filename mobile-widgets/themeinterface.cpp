// SPDX-License-Identifier: GPL-2.0
#include "themeinterface.h"
#include "qmlmanager.h"
#include "core/metrics.h"
#include "core/settings/qPrefDisplay.h"

QColor themeInterface::m_backgroundColor;
QColor themeInterface::m_contrastAccentColor;
QColor themeInterface::m_darkerPrimaryColor;
QColor themeInterface::m_darkerPrimaryTextColor;
QColor themeInterface::m_drawerColor;
QColor themeInterface::m_lightDrawerColor;
QColor themeInterface::m_lightPrimaryColor;
QColor themeInterface::m_lightPrimaryTextColor;
QColor themeInterface::m_primaryColor;
QColor themeInterface::m_primaryTextColor;
QColor themeInterface::m_secondaryTextColor;
QColor themeInterface::m_textColor;

double themeInterface::m_basePointSize;
double themeInterface::m_headingPointSize;
double themeInterface::m_regularPointSize;
double themeInterface::m_smallPointSize;
double themeInterface::m_titlePointSize;

QString themeInterface::m_currentTheme;
QString themeInterface::m_iconStyle;

const QColor themeInterface::m_blueBackgroundColor = "#eff0f1";
const QColor themeInterface::m_blueContrastAccentColor = "#FF5722";
const QColor themeInterface::m_blueDarkerPrimaryColor = "#303F9f";
const QColor themeInterface::m_blueDarkerPrimaryTextColor = "#ECECEC";
const QColor themeInterface::m_blueDrawerColor = "#FFFFFF";
const QColor themeInterface::m_blueLightDrawerColor = "#FFFFFF";
const QColor themeInterface::m_blueLightPrimaryColor = "#C5CAE9";
const QColor themeInterface::m_blueLightPrimaryTextColor = "#212121";
const QColor themeInterface::m_bluePrimaryColor = "#3F51B5";
const QColor themeInterface::m_bluePrimaryTextColor = "#FFFFFF";
const QColor themeInterface::m_blueSecondaryTextColor = "#757575";
const QColor themeInterface::m_blueTextColor = "#212121";

const QColor themeInterface::m_pinkBackgroundColor = "#eff0f1";
const QColor themeInterface::m_pinkContrastAccentColor = "#FF5722";
const QColor themeInterface::m_pinkDarkerPrimaryColor = "#C2185B";
const QColor themeInterface::m_pinkDarkerPrimaryTextColor = "#ECECEC";
const QColor themeInterface::m_pinkDrawerColor = "#FFFFFF";
const QColor themeInterface::m_pinkLightDrawerColor = "#FFFFFF";
const QColor themeInterface::m_pinkLightPrimaryColor = "#FFDDF4";
const QColor themeInterface::m_pinkLightPrimaryTextColor = "#212121";
const QColor themeInterface::m_pinkPrimaryColor = "#FF69B4";
const QColor themeInterface::m_pinkPrimaryTextColor = "#212121";
const QColor themeInterface::m_pinkSecondaryTextColor = "#757575";
const QColor themeInterface::m_pinkTextColor = "#212121";

const QColor themeInterface::m_darkBackgroundColor = "#303030";
const QColor themeInterface::m_darkContrastAccentColor = "#FF5722";
const QColor themeInterface::m_darkDarkerPrimaryColor = "#303F9f";
const QColor themeInterface::m_darkDarkerPrimaryTextColor = "#ECECEC";
const QColor themeInterface::m_darkDrawerColor = "#424242";
const QColor themeInterface::m_darkLightDrawerColor = "#FFFFFF";
const QColor themeInterface::m_darkLightPrimaryColor = "#C5CAE9";
const QColor themeInterface::m_darkLightPrimaryTextColor = "#ECECEC";
const QColor themeInterface::m_darkPrimaryColor = "#3F51B5";
const QColor themeInterface::m_darkPrimaryTextColor = "#ECECEC";
const QColor themeInterface::m_darkSecondaryTextColor = "#757575";
const QColor themeInterface::m_darkTextColor = "#ECECEC";

themeInterface *themeInterface::instance()
{
	static themeInterface *self = new themeInterface;
	return self;
}

void themeInterface::setup(QQmlContext *ct)
{
	// Register interface class
	ct->setContextProperty("ThemeNew", instance());

	// get current theme
	m_currentTheme = qPrefDisplay::theme();
	update_theme();

	// check system font
	m_basePointSize = defaultModelFont().pointSize();

	// set initial font size
	set_currentScale(qPrefDisplay::mobile_scale());
}

void themeInterface::set_currentTheme(const QString &theme)
{
	m_currentTheme = theme;
	qPrefDisplay::set_theme(m_currentTheme);
	instance()->update_theme();
	emit instance()->currentThemeChanged(theme);
}

double themeInterface::currentScale()
{
	return qPrefDisplay::mobile_scale();
}
void themeInterface::set_currentScale(double newScale)
{
	if (newScale != qPrefDisplay::mobile_scale()) {
		qPrefDisplay::set_mobile_scale(newScale);
		emit instance()->currentScaleChanged(qPrefDisplay::mobile_scale());
	}

	// Set current font size
	defaultModelFont().setPointSize(m_basePointSize * qPrefDisplay::mobile_scale());

	// adjust all used font sizes
	m_regularPointSize = defaultModelFont().pointSize();
	emit instance()->regularPointSizeChanged(m_regularPointSize);

	m_headingPointSize = m_regularPointSize * 1.2;
	emit instance()->headingPointSizeChanged(m_headingPointSize);

	m_smallPointSize = m_regularPointSize * 0.8;
	emit instance()->smallPointSizeChanged(m_smallPointSize);

	m_titlePointSize = m_regularPointSize * 1.5;
	emit instance()->titlePointSizeChanged(m_titlePointSize);
}

void themeInterface::update_theme()
{
	if (m_currentTheme == "Blue") {
		m_backgroundColor = m_blueBackgroundColor;
		m_contrastAccentColor = m_blueContrastAccentColor;
		m_darkerPrimaryColor = m_blueDarkerPrimaryColor;
		m_darkerPrimaryTextColor = m_blueDarkerPrimaryTextColor;
		m_drawerColor = m_blueDrawerColor;
		m_lightDrawerColor = m_blueDrawerColor;
		m_lightPrimaryColor = m_blueLightPrimaryColor;
		m_lightPrimaryTextColor = m_blueLightPrimaryTextColor;
		m_primaryColor = m_bluePrimaryColor;
		m_primaryTextColor = m_bluePrimaryTextColor;
		m_secondaryTextColor = m_blueSecondaryTextColor;
		m_textColor = m_blueTextColor;
		m_iconStyle = ":/icons";
	} else if (m_currentTheme == "Pink") {
		m_backgroundColor = m_pinkBackgroundColor;
		m_contrastAccentColor = m_pinkContrastAccentColor;
		m_darkerPrimaryColor = m_pinkDarkerPrimaryColor;
		m_darkerPrimaryTextColor = m_pinkDarkerPrimaryTextColor;
		m_drawerColor = m_pinkDrawerColor;
		m_lightDrawerColor = m_pinkDrawerColor;
		m_lightPrimaryColor = m_pinkLightPrimaryColor;
		m_lightPrimaryTextColor = m_pinkLightPrimaryTextColor;
		m_primaryColor = m_pinkPrimaryColor;
		m_primaryTextColor = m_pinkPrimaryTextColor;
		m_secondaryTextColor = m_pinkSecondaryTextColor;
		m_textColor = m_pinkTextColor;
		m_iconStyle = ":/icons";
	} else {
		m_backgroundColor = m_darkBackgroundColor;
		m_contrastAccentColor = m_darkContrastAccentColor;
		m_darkerPrimaryColor = m_darkDarkerPrimaryColor;
		m_darkerPrimaryTextColor = m_darkDarkerPrimaryTextColor;
		m_drawerColor = m_darkDrawerColor;
		m_lightDrawerColor = m_darkDrawerColor;
		m_lightPrimaryColor = m_darkLightPrimaryColor;
		m_lightPrimaryTextColor = m_darkLightPrimaryTextColor;
		m_primaryColor = m_darkPrimaryColor;
		m_primaryTextColor = m_darkPrimaryTextColor;
		m_secondaryTextColor = m_darkSecondaryTextColor;
		m_textColor = m_darkTextColor;
		m_iconStyle = ":/icons-dark";
	}
	emit instance()->backgroundColorChanged(m_backgroundColor);
	emit instance()->contrastAccentColorChanged(m_contrastAccentColor);
	emit instance()->darkerPrimaryColorChanged(m_darkerPrimaryColor);
	emit instance()->darkerPrimaryTextColorChanged(m_darkerPrimaryTextColor);
	emit instance()->drawerColorChanged(m_drawerColor);
	emit instance()->lightDrawerColorChanged(m_lightDrawerColor);
	emit instance()->lightPrimaryColorChanged(m_lightPrimaryColor);
	emit instance()->lightPrimaryTextColorChanged(m_lightPrimaryTextColor);
	emit instance()->primaryColorChanged(m_primaryColor);
	emit instance()->primaryTextColorChanged(m_primaryTextColor);
	emit instance()->secondaryTextColorChanged(m_secondaryTextColor);
	emit instance()->textColorChanged(m_textColor);
	emit instance()->iconStyleChanged(m_iconStyle);
}
