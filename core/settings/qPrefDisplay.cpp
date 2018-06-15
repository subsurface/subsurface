// SPDX-License-Identifier: GPL-2.0
#include "qPref.h"
#include "qPref_private.h"
#include "core/subsurface-string.h"

#include <QApplication>
#include <QFont>

qPrefDisplay::qPrefDisplay(QObject *parent) : QObject(parent)
{
}
qPrefDisplay *qPrefDisplay::instance()
{
	static qPrefDisplay *self = new qPrefDisplay;
	return self;
}

void qPrefDisplay::loadSync(bool doSync)
{
	disk_divelist_font(doSync);
	disk_font_size(doSync);
	disk_display_invalid_dives(doSync);
	disk_show_developer(doSync);
	disk_theme(doSync);
}

const QString qPrefDisplay::divelist_font() const
{
	return prefs.divelist_font;
}
void qPrefDisplay::set_divelist_font(const QString& value)
{
	QString newValue = value;
	if (value.contains(","))
		newValue = value.left(value.indexOf(","));

	if (newValue != prefs.divelist_font &&
		!subsurface_ignore_font(qPrintable(newValue))) {
		COPY_TXT(divelist_font, value);
		qApp->setFont(QFont(newValue));
		disk_divelist_font(true);
		emit divelist_font_changed(value);
	}
}
void qPrefDisplay::disk_divelist_font(bool doSync)
{
	LOADSYNC_TXT("/divelist_font", divelist_font);
}

double qPrefDisplay::font_size() const
{
	return prefs.font_size;
}
void qPrefDisplay::set_font_size(double value)
{
	if (value != prefs.font_size) {
		prefs.font_size = value;
		QFont defaultFont = qApp->font();
		defaultFont.setPointSizeF(prefs.font_size);
		qApp->setFont(defaultFont);
		disk_font_size(true);
		emit font_size_changed(value);
	}
}
void qPrefDisplay::disk_font_size(bool doSync)
{
	LOADSYNC_DOUBLE("/font_size", font_size);
}

bool qPrefDisplay::display_invalid_dives() const
{
	return prefs.display_invalid_dives;
}
void qPrefDisplay::set_display_invalid_dives(bool value)
{
	if (value != prefs.display_invalid_dives) {
		prefs.display_invalid_dives = value;
		disk_display_invalid_dives(true);
		emit display_invalid_dives_changed(value);
	}
}
void qPrefDisplay::disk_display_invalid_dives(bool doSync)
{
	LOADSYNC_BOOL("/displayinvalid", display_invalid_dives);
}

bool qPrefDisplay::show_developer() const
{
	return prefs.show_developer;
}
void qPrefDisplay::set_show_developer(bool value)
{
	if (value != prefs.show_developer) {
		prefs.show_developer = value;
		disk_show_developer(true);
		emit disk_show_developer(value);
	}
}
void qPrefDisplay::disk_show_developer(bool doSync)
{
	LOADSYNC_BOOL("/showDeveloper", show_developer);
}

const QString qPrefDisplay::theme() const
{
	return prefs.theme;
}
void qPrefDisplay::set_theme(const QString& value)
{
	if (value != prefs.theme) {
		COPY_TXT(theme, value);
		disk_theme(true);
		emit theme_changed(value);
	}
}
void qPrefDisplay::disk_theme(bool doSync)
{
	LOADSYNC_TXT("/theme", theme);
}
