// SPDX-License-Identifier: GPL-2.0
#include "themeinterface.h"
#include "core/settings/qPrefDisplay.h"

themeInterface *themeInterface::instance()
{
	static themeInterface *self = new themeInterface;
	return self;
}

void themeInterface::setup()
{
	// get current theme
	m_currentTheme = qPrefDisplay::theme();
	update_theme();
}

void themeInterface::set_currentTheme(const QString &theme)
{
	qPrefDisplay::set_theme(m_currentTheme);
	emit currentThemeChanged(theme);
}

void themeInterface::update_theme()
{
}


