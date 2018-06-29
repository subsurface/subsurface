// SPDX-License-Identifier: GPL-2.0
#include "qPrefDisplay.h"
#include "qPref_private.h"
#include "core/subsurface-string.h"

#include <QApplication>
#include <QFont>


qPrefDisplay *qPrefDisplay::m_instance = NULL;
qPrefDisplay *qPrefDisplay::instance()
{
	if (!m_instance)
		m_instance = new qPrefDisplay;
	return m_instance;
}

void qPrefDisplay::loadSync(bool doSync)
{
	diskDivelistFont(doSync);
	diskFontSize(doSync);
	diskDisplayInvalidDives(doSync);
	diskShowDeveloper(doSync);
	diskTheme(doSync);
}


const QString qPrefDisplay::divelistFont() const
{
	return prefs.divelist_font;
}
void qPrefDisplay::setDivelistFont(const QString& value)
{
	QString newValue = value;
	if (value.contains(","))
		newValue = value.left(value.indexOf(","));

	if (newValue != prefs.divelist_font &&
		!subsurface_ignore_font(qPrintable(newValue))) {
		COPY_TXT(divelist_font, value);
		qApp->setFont(QFont(newValue));
		diskDivelistFont(true);
		emit diveListFontChanged(value);
	}
}
void qPrefDisplay::diskDivelistFont(bool doSync)
{
	LOADSYNC_TXT("/divelist_font", divelist_font);
}


double qPrefDisplay::fontSize() const
{
	return prefs.font_size;
}
void qPrefDisplay::setFontSize(double value)
{
	if (value != prefs.font_size) {
		prefs.font_size = value;
		QFont defaultFont = qApp->font();
		defaultFont.setPointSizeF(prefs.font_size);
		qApp->setFont(defaultFont);
		diskFontSize(true);
		emit fontSizeChanged(value);
	}
}
void qPrefDisplay::diskFontSize(bool doSync)
{
	LOADSYNC_DOUBLE("/font_size", font_size);
}


bool qPrefDisplay::displayInvalidDives() const
{
	return prefs.display_invalid_dives;
}
void qPrefDisplay::setDisplayInvalidDives(bool value)
{
	if (value != prefs.display_invalid_dives) {
		prefs.display_invalid_dives = value;
		diskDisplayInvalidDives(true);
		emit displayInvalidDivesChanged(value);
	}
}
void qPrefDisplay::diskDisplayInvalidDives(bool doSync)
{
	LOADSYNC_BOOL("/displayinvalid", display_invalid_dives);
}


bool qPrefDisplay::showDeveloper() const
{
	return prefs.showDeveloper;
}
void qPrefDisplay::setShowDeveloper(bool value)
{
	if (value != prefs.showDeveloper) {
		prefs.showDeveloper = value;
		diskShowDeveloper(true);
		emit diskShowDeveloper(value);
	}
}
void qPrefDisplay::diskShowDeveloper(bool doSync)
{
	if (!doSync) {
		QSettings s;
		// get the font from the settings or our defaults
		// respect the system default font size if none is explicitly set
		QFont defaultFont = s.value("divelist_font", prefs.divelist_font).value<QFont>();
		if (IS_FP_SAME(system_divelist_default_font_size, -1.0)) {
			prefs.font_size = qApp->font().pointSizeF();
			system_divelist_default_font_size = prefs.font_size; // this way we don't save it on exit
		}
		prefs.font_size = s.value("font_size", prefs.font_size).toFloat();
		// painful effort to ignore previous default fonts on Windows - ridiculous
		QString fontName = defaultFont.toString();
		if (fontName.contains(","))
			fontName = fontName.left(fontName.indexOf(","));
		if (subsurface_ignore_font(qPrintable(fontName))) {
			defaultFont = QFont(prefs.divelist_font);
		} else
			COPY_TXT(divelist_font, fontName);
		defaultFont.setPointSizeF(prefs.font_size);
		qApp->setFont(defaultFont);
	}
	else
		LOADSYNC_BOOL("/showDeveloper", showDeveloper);
}


const QString qPrefDisplay::theme() const
{
	return prefs.theme;
}
void qPrefDisplay::setTheme(const QString& value)
{
	if (value != prefs.theme) {
		COPY_TXT(theme, value);
		diskTheme(true);
		emit themeChanged(value);
	}
}
void qPrefDisplay::diskTheme(bool doSync)
{
	LOADSYNC_TXT("/theme", theme);
}
