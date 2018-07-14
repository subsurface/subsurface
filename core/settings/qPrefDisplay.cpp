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
void qPrefDisplay::disk_divelist_font(bool doSync)
{
	LOADSYNC_TXT("/divelist_font", divelist_font);
	if (!doSync)
		setCorrectFont();
}

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
void qPrefDisplay::disk_font_size(bool doSync)
{
	LOADSYNC_DOUBLE("/font_size", font_size);
	if (!doSync)
		setCorrectFont();
}

HANDLE_PREFERENCE_BOOL(Display, "/displayinvalid", display_invalid_dives);

HANDLE_PREFERENCE_BOOL(Display, "/show_developer", show_developer);

HANDLE_PREFERENCE_TXT(Display, "/theme", theme);


void qPrefDisplay::setCorrectFont()
{
	// get the font from the settings or our defaults
	// respect the system default font size if none is explicitly set
	QFont defaultFont(prefs.divelist_font);
	if (IS_FP_SAME(system_divelist_default_font_size, -1.0)) {
		prefs.font_size = qApp->font().pointSizeF();
		system_divelist_default_font_size = prefs.font_size; // this way we don't save it on exit
	}
	// painful effort to ignore previous default fonts on Windows - ridiculous
	QString fontName = defaultFont.toString();
	if (fontName.contains(","))
		fontName = fontName.left(fontName.indexOf(","));
	if (subsurface_ignore_font(qPrintable(fontName)))
		defaultFont = QFont(prefs.divelist_font);
	else
		COPY_TXT(divelist_font, fontName);
	defaultFont.setPointSizeF(prefs.font_size);
	qApp->setFont(defaultFont);
}
