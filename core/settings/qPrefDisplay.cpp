// SPDX-License-Identifier: GPL-2.0
#include "qPrefDisplay.h"
#include "qPrefPrivate.h"
#include "core/subsurface-float.h"

#include <QApplication>
#include <QFont>

static const QString group = QStringLiteral("Display");

QPointF qPrefDisplay::st_tooltip_position;
static const QPointF st_tooltip_position_default = QPointF(0,0);

QString qPrefDisplay::st_lastDir;
static const QString st_lastDir_default;

QString qPrefDisplay::st_theme;
static const QString st_theme_default = QStringLiteral("Blue");

QByteArray qPrefDisplay::st_mainSplitter;
static const QByteArray st_mainSplitter_default;

QByteArray qPrefDisplay::st_topSplitter;
static const QByteArray st_topSplitter_default;

QByteArray qPrefDisplay::st_bottomSplitter;
static const QByteArray st_bottomSplitter_default;

bool qPrefDisplay::st_maximized;
static bool st_maximized_default = false;

QByteArray qPrefDisplay::st_geometry;
static const QByteArray st_geometry_default;

QByteArray qPrefDisplay::st_windowState;
static const QByteArray st_windowState_default;

int qPrefDisplay::st_lastState;
static int st_lastState_default = false;

bool qPrefDisplay::st_singleColumnPortrait;
static bool st_singleColumnPortrait_default = false;

qPrefDisplay *qPrefDisplay::instance()
{
	static qPrefDisplay *self = new qPrefDisplay;
	return self;
}

void qPrefDisplay::loadSync(bool doSync)
{
	disk_animation_speed(doSync);
	disk_divelist_font(doSync);
	disk_font_size(doSync);
	disk_mobile_scale(doSync);
	disk_display_invalid_dives(doSync);
	disk_show_developer(doSync);
	if (!doSync) {
		load_tooltip_position();
		load_theme();
		load_mainSplitter();
		load_topSplitter();
		load_bottomSplitter();
		load_maximized();
		load_geometry();
		load_windowState();
		load_lastState();
		load_singleColumnPortrait();
	}
	disk_three_m_based_grid(doSync);
	disk_map_short_names(doSync);
}

void qPrefDisplay::set_divelist_font(const QString &value)
{
	QString newValue = value;
	if (value.contains(","))
		newValue = value.left(value.indexOf(","));

	if (newValue.toStdString() != prefs.divelist_font &&
	    !subsurface_ignore_font(newValue.toStdString())) {
		prefs.divelist_font = value.toStdString();
		disk_divelist_font(true);

		qApp->setFont(QFont(newValue));
		emit instance()->divelist_fontChanged(value);
	}
}
void qPrefDisplay::disk_divelist_font(bool doSync)
{
	if (doSync)
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "divelist_font"), prefs.divelist_font, default_prefs.divelist_font);
	else
		setCorrectFont();
}

void qPrefDisplay::set_font_size(double value)
{
	if (!nearly_equal(value, prefs.font_size)) {
		prefs.font_size = value;
		disk_font_size(true);

		QFont defaultFont = qApp->font();
		defaultFont.setPointSizeF(prefs.font_size * prefs.mobile_scale);
		qApp->setFont(defaultFont);
		emit instance()->font_sizeChanged(value);
	}
}

void qPrefDisplay::disk_font_size(bool doSync)
{
	// inverted logic compared to the other disk_xxx functions
	if (!doSync)
		setCorrectFont();
#if !defined(SUBSURFACE_MOBILE)
	// we never want to save the font_size to disk - we always want to grab that from the system default
	else
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "font_size"), prefs.font_size, default_prefs.font_size);
#endif
}

void qPrefDisplay::set_mobile_scale(double value)
{
	if (!nearly_equal(value, prefs.mobile_scale)) {
		prefs.mobile_scale = value;
		disk_mobile_scale(true);

		QFont defaultFont = qApp->font();
		defaultFont.setPointSizeF(prefs.font_size * prefs.mobile_scale);
		qApp->setFont(defaultFont);
		emit instance()->mobile_scaleChanged(value);
		emit instance()->font_sizeChanged(value);
	}
}

void qPrefDisplay::disk_mobile_scale(bool doSync)
{
	if (doSync) {
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "mobile_scale"), prefs.mobile_scale, default_prefs.mobile_scale);
	} else {
		prefs.mobile_scale = qPrefPrivate::propValue(keyFromGroupAndName(group, "mobile_scale"), default_prefs.mobile_scale).toDouble();
		setCorrectFont();
	}
}

//JAN static const QString group = QStringLiteral("Animations");
HANDLE_PREFERENCE_INT(Display, "animation_speed", animation_speed);

HANDLE_PREFERENCE_BOOL(Display, "displayinvalid", display_invalid_dives);

HANDLE_PREFERENCE_BOOL(Display, "show_developer", show_developer);

HANDLE_PREFERENCE_BOOL(Display, "three_m_based_grid", three_m_based_grid);

HANDLE_PREFERENCE_BOOL(Display, "map_short_names", map_short_names);

void qPrefDisplay::setCorrectFont()
{
	// get the font from the settings or our defaults
	// respect the system default font size if none is explicitly set
	QFont defaultFont = qPrefPrivate::propValue(keyFromGroupAndName(group, "divelist_font"), prefs.divelist_font).value<QFont>();
	if (nearly_equal(system_divelist_default_font_size, -1.0)) {
		prefs.font_size = qApp->font().pointSizeF();
		system_divelist_default_font_size = prefs.font_size; // this way we don't save it on exit
	}

	prefs.font_size = qPrefPrivate::propValue(keyFromGroupAndName(group, "font_size"), prefs.font_size).toFloat();

	// painful effort to ignore previous default fonts on Windows - ridiculous
	QString fontName = defaultFont.toString();
	if (fontName.contains(","))
		fontName = fontName.left(fontName.indexOf(","));
	if (subsurface_ignore_font(fontName.toStdString()))
		defaultFont = QFont(QString::fromStdString(prefs.divelist_font));
	else
		prefs.divelist_font = fontName.toStdString();
	defaultFont.setPointSizeF(prefs.font_size * prefs.mobile_scale);
	qApp->setFont(defaultFont);

	prefs.display_invalid_dives = qPrefPrivate::propValue(keyFromGroupAndName(group, "displayinvalid"), default_prefs.display_invalid_dives).toBool();
}

HANDLE_PROP_QSTRING(Display, "FileDialog/LastDir", lastDir);

HANDLE_PROP_QSTRING(Display, "Theme/currentTheme", theme);

HANDLE_PROP_QPOINTF(Display, "ProfileMap/tooltip_position", tooltip_position);

HANDLE_PROP_QBYTEARRAY(Display, "MainWindow/mainSplitter", mainSplitter);

HANDLE_PROP_QBYTEARRAY(Display, "MainWindow/topSplitter", topSplitter);

HANDLE_PROP_QBYTEARRAY(Display, "MainWindow/bottomSplitter", bottomSplitter);

HANDLE_PROP_BOOL(Display, "MainWindow/maximized", maximized);

HANDLE_PROP_QBYTEARRAY(Display, "MainWindow/geometry", geometry);

HANDLE_PROP_QBYTEARRAY(Display, "MainWindow/windowState", windowState);

HANDLE_PROP_INT(Display, "MainWindow/lastState", lastState);

HANDLE_PROP_BOOL(Display, "singleColumnPortrait", singleColumnPortrait);
