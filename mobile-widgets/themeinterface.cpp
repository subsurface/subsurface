// SPDX-License-Identifier: GPL-2.0
#include "themeinterface.h"
#include "qmlmanager.h"
#include "core/metrics.h"
#include "core/settings/qPrefDisplay.h"

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
	instance()->m_currentTheme = qPrefDisplay::theme();
	instance()->update_theme();

	// check system font
	instance()->m_basePointSize = defaultModelFont().pointSize();

	// set initial font size
	instance()->set_currentScale(qPrefDisplay::mobile_scale());
}

void themeInterface::set_currentTheme(const QString &theme)
{
	m_currentTheme = theme;
	qPrefDisplay::set_theme(m_currentTheme);
	update_theme();
	emit currentThemeChanged(theme);
}

double themeInterface::currentScale()
{
	return qPrefDisplay::mobile_scale();
}
void themeInterface::set_currentScale(double newScale)
{
	if (newScale != qPrefDisplay::mobile_scale()) {
		qPrefDisplay::set_mobile_scale(newScale);
		emit currentScaleChanged(qPrefDisplay::mobile_scale());
	}

	// Set current font size
	defaultModelFont().setPointSize(m_basePointSize * qPrefDisplay::mobile_scale());

	// adjust all used font sizes
	m_regularPointSize = defaultModelFont().pointSize();
	emit regularPointSizeChanged(m_regularPointSize);

	m_headingPointSize = m_regularPointSize * 1.2;
	emit headingPointSizeChanged(m_headingPointSize);

	m_smallPointSize = m_regularPointSize * 0.8;
	emit smallPointSizeChanged(m_smallPointSize);

	m_titlePointSize = m_regularPointSize * 1.5;
	emit titlePointSizeChanged(m_titlePointSize);
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
}


