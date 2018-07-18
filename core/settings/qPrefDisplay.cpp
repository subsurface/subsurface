// SPDX-License-Identifier: GPL-2.0
#include "qPref.h"
#include "qPrefPrivate.h"
#include "core/subsurface-string.h"

#include <QApplication>
#include <QFont>

static const QString group = QStringLiteral("Display");

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

void qPrefDisplay::set_divelist_font(const QString& value)
{
	QString newValue = value;
	if (value.contains(","))
		newValue = value.left(value.indexOf(","));

	if (newValue != prefs.divelist_font &&
		!subsurface_ignore_font(qPrintable(newValue))) {
		qPrefPrivate::copy_txt(&prefs.divelist_font, value);
		disk_divelist_font(true);

		qApp->setFont(QFont(newValue));
		emit divelist_font_changed(value);
	}
}
void qPrefDisplay::disk_divelist_font(bool doSync)
{
	if (doSync)
		qPrefPrivate::instance()->setting.setValue(group + "/divelist_font", prefs.divelist_font);
	else
		setCorrectFont();
}

void qPrefDisplay::set_font_size(double value)
{
	if (value != prefs.font_size) {
		prefs.font_size = value;
		disk_font_size(true);

		QFont defaultFont = qApp->font();
		defaultFont.setPointSizeF(prefs.font_size);
		qApp->setFont(defaultFont);
		emit font_size_changed(value);
	}
}
void qPrefDisplay::disk_font_size(bool doSync)
{
	if (doSync)
		qPrefPrivate::instance()->setting.setValue(group + "/font_size", prefs.font_size);
	else
		setCorrectFont();
}

HANDLE_PREFERENCE_BOOL(Display, "/displayinvalid", display_invalid_dives);

HANDLE_PREFERENCE_BOOL(Display, "/show_developer", show_developer);

HANDLE_PREFERENCE_TXT(Display, "/theme", theme);


void qPrefDisplay::setCorrectFont()
{
	QSettings s;
	QVariant v;

	// get the font from the settings or our defaults
	// respect the system default font size if none is explicitly set
	QFont defaultFont = s.value(group + "/divelist_font", prefs.divelist_font).value<QFont>();
	if (IS_FP_SAME(system_divelist_default_font_size, -1.0)) {
		prefs.font_size = qApp->font().pointSizeF();
		system_divelist_default_font_size = prefs.font_size; // this way we don't save it on exit
	}

	prefs.font_size = s.value(group + "/font_size", prefs.font_size).toFloat();
	// painful effort to ignore previous default fonts on Windows - ridiculous
	QString fontName = defaultFont.toString();
	if (fontName.contains(","))
		fontName = fontName.left(fontName.indexOf(","));
	if (subsurface_ignore_font(qPrintable(fontName))) {
		defaultFont = QFont(prefs.divelist_font);
	} else {
		free((void *)prefs.divelist_font);
		prefs.divelist_font = copy_qstring(fontName);
	}
	defaultFont.setPointSizeF(prefs.font_size);
	qApp->setFont(defaultFont);

	prefs.display_invalid_dives = qPrefPrivate::instance()->setting.value(group + "/displayinvalid", default_prefs.display_invalid_dives).toBool();
}
