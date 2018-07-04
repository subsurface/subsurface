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

GET_PREFERENCE_TXT(Display, divelist_font);
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
DISK_LOADSYNC_TXT(Display, "/divelist_font", divelist_font);

GET_PREFERENCE_DOUBLE(Display, font_size);
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
DISK_LOADSYNC_DOUBLE(Display, "/font_size", font_size);

HANDLE_PREFERENCE_BOOL(Display, "/displayinvalid", display_invalid_dives);

HANDLE_PREFERENCE_BOOL(Display, "/show_developer", show_developer);

HANDLE_PREFERENCE_TXT(Display, "/theme", theme);
