// SPDX-License-Identifier: GPL-2.0
#include "themeinterface.h"
#include "core/subsurface-string.h"
#include "qmlmanager.h"
#include "core/metrics.h"
#include "core/settings/qPrefDisplay.h"
#include <QFontInfo>

static const QColor BLUE_BACKGROUND_COLOR = "#eff0f1";
static const QColor BLUE_CONTRAST_ACCENT_COLOR = "#FF5722";
static const QColor BLUE_DARKER_PRIMARY_COLOR = "#303F9f";
static const QColor BLUE_DARKER_PRIMARY_TEXT_COLOR = "#ECECEC";
static const QColor BLUE_DRAWER_COLOR = "#FFFFFF";
static const QColor BLUE_LIGHT_DRAWER_COLOR = "#FFFFFF";
static const QColor BLUE_LIGHT_PRIMARY_COLOR = "#C5CAE9";
static const QColor BLUE_LIGHT_PRIMARY_TEXT_COLOR = "#212121";
static const QColor BLUE_PRIMARY_COLOR = "#3F51B5";
static const QColor BLUE_PRIMARY_TEXT_COLOR = "#FFFFFF";
static const QColor BLUE_SECONDARY_TEXT_COLOR = "#757575";
static const QColor BLUE_TEXT_COLOR = "#212121";

static const QColor PINK_BACKGROUND_COLOR = "#eff0f1";
static const QColor PINK_CONTRAST_ACCENT_COLOR = "#FF5722";
static const QColor PINK_DARKER_PRIMARY_COLOR = "#C2185B";
static const QColor PINK_DARKER_PRIMARY_TEXT_COLOR = "#ECECEC";
static const QColor PINK_DRAWER_COLOR = "#FFFFFF";
static const QColor PINK_LIGHT_DRAWER_COLOR = "#FFFFFF";
static const QColor PINK_LIGHT_PRIMARY_COLOR = "#FFDDF4";
static const QColor PINK_LIGHT_PRIMARY_TEXT_COLOR = "#212121";
static const QColor PINK_PRIMARY_COLOR = "#FF69B4";
static const QColor PINK_PRIMARY_TEXT_COLOR = "#212121";
static const QColor PINK_SECONDARY_TEXT_COLOR = "#757575";
static const QColor PINK_TEXT_COLOR = "#212121";

static const QColor DARK_BACKGROUND_COLOR = "#303030";
static const QColor DARK_CONTRAST_ACCENT_COLOR = "#FF5722";
static const QColor DARK_DARKER_PRIMARY_COLOR = "#303F9f";
static const QColor DARK_DARKER_PRIMARY_TEXT_COLOR = "#ECECEC";
static const QColor DARK_DRAWER_COLOR = "#424242";
static const QColor DARK_LIGHT_DRAWER_COLOR = "#FFFFFF";
static const QColor DARK_LIGHT_PRIMARY_COLOR = "#303050";
static const QColor DARK_LIGHT_PRIMARY_TEXT_COLOR = "#ECECEC";
static const QColor DARK_PRIMARY_COLOR = "#3F51B5";
static const QColor DARK_PRIMARY_TEXT_COLOR = "#ECECEC";
static const QColor DARK_SECONDARY_TEXT_COLOR = "#757575";
static const QColor DARK_TEXT_COLOR = "#ECECEC";

ThemeInterface *ThemeInterface::instance()
{
	static ThemeInterface *self = new ThemeInterface;
	return self;
}

ThemeInterface::ThemeInterface()
{
	// get current theme
	m_currentTheme = qPrefDisplay::theme();
	update_theme();
	m_basePointSize = -1.0; // simply a placeholder to declare 'this isn't set, yet'
}

void ThemeInterface::set_currentTheme(const QString &theme)
{
	m_currentTheme = theme;
	qPrefDisplay::set_theme(m_currentTheme);
	update_theme();
	emit currentThemeChanged();
}

void ThemeInterface::setInitialFontSize(double fontSize)
{
	m_basePointSize = fontSize;
	set_currentScale(qPrefDisplay::mobile_scale());
}

double ThemeInterface::currentScale()
{
	return qPrefDisplay::mobile_scale();
}

void ThemeInterface::set_currentScale(double newScale)
{
	static bool needSignals = true; // make sure the signals fire the first time

	if (!IS_FP_SAME(newScale, qPrefDisplay::mobile_scale())) {
		qPrefDisplay::set_mobile_scale(newScale);
		emit currentScaleChanged();
		needSignals = true;
	}
	if (needSignals) {
		// adjust all used font sizes
		m_regularPointSize = m_basePointSize * newScale;
		defaultModelFont().setPointSizeF(m_regularPointSize);
		emit regularPointSizeChanged();

		m_headingPointSize = m_regularPointSize * 1.2;
		emit headingPointSizeChanged();

		m_smallPointSize = m_regularPointSize * 0.8;
		emit smallPointSizeChanged();

		m_titlePointSize = m_regularPointSize * 1.5;
		emit titlePointSizeChanged();

		needSignals = false;
	}
}

void ThemeInterface::update_theme()
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
	}
	emit backgroundColorChanged();
	emit contrastAccentColorChanged();
	emit darkerPrimaryColorChanged();
	emit darkerPrimaryTextColorChanged();
	emit drawerColorChanged();
	emit lightDrawerColorChanged();
	emit lightPrimaryColorChanged();
	emit lightPrimaryTextColorChanged();
	emit primaryColorChanged();
	emit primaryTextColorChanged();
	emit secondaryTextColorChanged();
	emit textColorChanged();
}
