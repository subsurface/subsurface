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

const QColor BLUE_BACKGROUND_COLOR = "#eff0f1";
const QColor BLUE_CONTRAST_ACCENT_COLOR = "#FF5722";
const QColor BLUE_DARKER_PRIMARY_COLOR = "#303F9f";
const QColor BLUE_DARKER_PRIMARY_TEXT_COLOR = "#ECECEC";
const QColor BLUE_DRAWER_COLOR = "#FFFFFF";
const QColor BLUE_LIGHT_DRAWER_COLOR = "#FFFFFF";
const QColor BLUE_LIGHT_PRIMARY_COLOR = "#C5CAE9";
const QColor BLUE_LIGHT_PRIMARY_TEXT_COLOR = "#212121";
const QColor BLUE_PRIMARY_COLOR = "#3F51B5";
const QColor BLUE_PRIMARY_TEXT_COLOR = "#FFFFFF";
const QColor BLUE_SECONDARY_TEXT_COLOR = "#757575";
const QColor BLUE_TEXT_COLOR = "#212121";

const QColor PINK_BACKGROUND_COLOR = "#eff0f1";
const QColor PINK_CONTRAST_ACCENT_COLOR = "#FF5722";
const QColor PINK_DARKER_PRIMARY_COLOR = "#C2185B";
const QColor PINK_DARKER_PRIMARY_TEXT_COLOR = "#ECECEC";
const QColor PINK_DRAWER_COLOR = "#FFFFFF";
const QColor PINK_LIGHT_DRAWER_COLOR = "#FFFFFF";
const QColor PINK_LIGHT_PRIMARY_COLOR = "#FFDDF4";
const QColor PINK_LIGHT_PRIMARY_TEXT_COLOR = "#212121";
const QColor PINK_PRIMARY_COLOR = "#FF69B4";
const QColor PINK_PRIMARY_TEXT_COLOR = "#212121";
const QColor PINK_SECONDARY_TEXT_COLOR = "#757575";
const QColor PINK_TEXT_COLOR = "#212121";

const QColor DARK_BACKGROUND_COLOR = "#303030";
const QColor DARK_CONTRAST_ACCENT_COLOR = "#FF5722";
const QColor DARK_DARKER_PRIMARY_COLOR = "#303F9f";
const QColor DARK_DARKER_PRIMARY_TEXT_COLOR = "#ECECEC";
const QColor DARK_DRAWER_COLOR = "#424242";
const QColor DARK_LIGHT_DRAWER_COLOR = "#FFFFFF";
const QColor DARK_LIGHT_PRIMARY_COLOR = "#C5CAE9";
const QColor DARK_LIGHT_PRIMARY_TEXT_COLOR = "#ECECEC";
const QColor DARK_PRIMARY_COLOR = "#3F51B5";
const QColor DARK_PRIMARY_TEXT_COLOR = "#ECECEC";
const QColor DARK_SECONDARY_TEXT_COLOR = "#757575";
const QColor DARK_TEXT_COLOR = "#ECECEC";

themeInterface *themeInterface::instance()
{
	static themeInterface *self = new themeInterface;
	return self;
}

void themeInterface::setup(QQmlContext *ct)
{
	// Register interface class
	ct->setContextProperty("subsurfaceTheme", instance());

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
		m_backgroundColor = BLUE_BACKGROUND_COLOR;
		m_contrastAccentColor = BLUE_CONTRAST_ACCENT_COLOR;
		m_darkerPrimaryColor = BLUE_DARKER_PRIMARY_COLOR;
		m_darkerPrimaryTextColor = BLUE_DARKER_PRIMARY_TEXT_COLOR;
		m_drawerColor = BLUE_DRAWER_COLOR;
		m_lightDrawerColor = BLUE_LIGHT_DRAWER_COLOR;
		m_lightPrimaryColor = BLUE_LIGHT_PRIMARY_COLOR;
		m_lightPrimaryTextColor = BLUE_LIGHT_PRIMARY_TEXT_COLOR;
		m_primaryColor = BLUE_PRIMARY_COLOR;
		m_primaryTextColor = BLUE_PRIMARY_TEXT_COLOR;
		m_secondaryTextColor = BLUE_SECONDARY_TEXT_COLOR;
		m_textColor = BLUE_TEXT_COLOR;
		m_iconStyle = ":/icons";
	} else if (m_currentTheme == "Pink") {
		m_backgroundColor = PINK_BACKGROUND_COLOR;
		m_contrastAccentColor = PINK_CONTRAST_ACCENT_COLOR;
		m_darkerPrimaryColor = PINK_DARKER_PRIMARY_COLOR;
		m_darkerPrimaryTextColor = PINK_DARKER_PRIMARY_TEXT_COLOR;
		m_drawerColor = PINK_DRAWER_COLOR;
		m_lightDrawerColor = PINK_LIGHT_DRAWER_COLOR;
		m_lightPrimaryColor = PINK_LIGHT_PRIMARY_COLOR;
		m_lightPrimaryTextColor = PINK_LIGHT_PRIMARY_TEXT_COLOR;
		m_primaryColor = PINK_PRIMARY_COLOR;
		m_primaryTextColor = PINK_PRIMARY_TEXT_COLOR;
		m_secondaryTextColor = PINK_SECONDARY_TEXT_COLOR;
		m_textColor = PINK_TEXT_COLOR;
		m_iconStyle = ":/icons";
	} else {
		m_backgroundColor = DARK_BACKGROUND_COLOR;
		m_contrastAccentColor = DARK_CONTRAST_ACCENT_COLOR;
		m_darkerPrimaryColor = DARK_DARKER_PRIMARY_COLOR;
		m_darkerPrimaryTextColor = DARK_DARKER_PRIMARY_TEXT_COLOR;
		m_drawerColor = DARK_DRAWER_COLOR;
		m_lightDrawerColor = DARK_LIGHT_DRAWER_COLOR;
		m_lightPrimaryColor = DARK_LIGHT_PRIMARY_COLOR;
		m_lightPrimaryTextColor = DARK_LIGHT_PRIMARY_TEXT_COLOR;
		m_primaryColor = DARK_PRIMARY_COLOR;
		m_primaryTextColor = DARK_PRIMARY_TEXT_COLOR;
		m_secondaryTextColor = DARK_SECONDARY_TEXT_COLOR;
		m_textColor = DARK_TEXT_COLOR;
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
