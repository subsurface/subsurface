// SPDX-License-Identifier: GPL-2.0
#include "qPrefDisplay.h"
#include "qPrefPrivate.h"
#include "core/subsurface-float.h"

#include <QApplication>
#include <QFont>
#include <QScreen>
#if !defined(SUBSURFACE_MOBILE) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
#include <QPalette>
#include <QStyleHints>
#include <optional>
#endif

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

#if !defined(SUBSURFACE_MOBILE) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
QString qPrefDisplay::st_color_scheme;
static const QString st_color_scheme_default = QStringLiteral("system");
#endif

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
#if !defined(SUBSURFACE_MOBILE) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
		load_color_scheme();
#endif
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
#if !defined(SUBSURFACE_MOBILE)
		// pick a sensible default based on screen dimensions
		QSize screen = QGuiApplication::primaryScreen()->size();
		if (screen.width() < 1024 || screen.height() < 768)
			prefs.font_size = 9.0;
		else if (screen.width() > 2560 && screen.height() > 1440)
			prefs.font_size = 13.0;
		else
			prefs.font_size = 11.0;
#else
		prefs.font_size = qApp->font().pointSizeF();
#endif
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

#if !defined(SUBSURFACE_MOBILE) && QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
QString qPrefDisplay::validated_color_scheme(const QString &value)
{
	if (value == QLatin1String("light") || value == QLatin1String("dark") || value == QLatin1String("system"))
		return value;
	return st_color_scheme_default;
}

// Qt only forwards QStyleHints::setColorScheme() to the platform theme, and
// themes without support for it (for example the generic Unix theme used on
// WSLg or when no desktop environment is running) ignore the request. It also
// takes effect asynchronously where it is supported, so it cannot be used to
// tell whether an override worked. To make an explicit Light or Dark choice
// visibly effective everywhere, the matching palette is also set on the
// application. "Use system" removes both again.
static QPalette color_scheme_palette(Qt::ColorScheme scheme)
{
	const bool dark = scheme == Qt::ColorScheme::Dark;
	const QColor window = dark ? QColor(0x35, 0x35, 0x35) : QColor(0xef, 0xef, 0xef);
	const QColor base = dark ? QColor(0x23, 0x23, 0x23) : QColor(0xff, 0xff, 0xff);
	const QColor alternateBase = dark ? QColor(0x2d, 0x2d, 0x2d) : QColor(0xf7, 0xf7, 0xf7);
	const QColor text = dark ? QColor(0xf0, 0xf0, 0xf0) : QColor(0x00, 0x00, 0x00);
	const QColor disabledText = dark ? QColor(0x80, 0x80, 0x80) : QColor(0xbe, 0xbe, 0xbe);
	const QColor highlight = dark ? QColor(0x2a, 0x82, 0xda) : QColor(0x30, 0x8c, 0xc6);
	const QColor disabledHighlight = dark ? QColor(0x50, 0x50, 0x50) : QColor(0x91, 0x91, 0x91);
	const QColor link = dark ? QColor(0x5c, 0xaa, 0xf0) : QColor(0x00, 0x00, 0xff);
	const QColor linkVisited = dark ? QColor(0xb0, 0x8c, 0xe6) : QColor(0xff, 0x00, 0xff);
	const QColor toolTipBase = dark ? QColor(0x45, 0x45, 0x45) : QColor(0xff, 0xff, 0xdc);
	QColor placeholder = text;
	placeholder.setAlpha(128);

	// The two colour constructor derives the bevel roles (Light, Midlight, Dark,
	// Mid, Shadow) that styles use to draw frames from the button colour
	QPalette palette(window, window);
	palette.setColor(QPalette::WindowText, text);
	palette.setColor(QPalette::Text, text);
	palette.setColor(QPalette::ButtonText, text);
	palette.setColor(QPalette::BrightText, QColor(0xff, 0x00, 0x00));
	palette.setColor(QPalette::Base, base);
	palette.setColor(QPalette::AlternateBase, alternateBase);
	palette.setColor(QPalette::ToolTipBase, toolTipBase);
	palette.setColor(QPalette::ToolTipText, text);
	palette.setColor(QPalette::PlaceholderText, placeholder);
	palette.setColor(QPalette::Highlight, highlight);
	palette.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
	palette.setColor(QPalette::Accent, highlight);
	palette.setColor(QPalette::Link, link);
	palette.setColor(QPalette::LinkVisited, linkVisited);
	palette.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
	palette.setColor(QPalette::Disabled, QPalette::Text, disabledText);
	palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
	palette.setColor(QPalette::Disabled, QPalette::Highlight, disabledHighlight);
	palette.setColor(QPalette::Disabled, QPalette::Accent, disabledHighlight);

	// Mark every role as explicitly set. Roles left unset would be filled in
	// from the platform theme, which is what is being overridden.
	for (int group = 0; group < QPalette::NColorGroups; group++) {
		for (int role = 0; role < QPalette::NColorRoles; role++) {
			const QPalette::ColorGroup g = static_cast<QPalette::ColorGroup>(group);
			const QPalette::ColorRole r = static_cast<QPalette::ColorRole>(role);
			palette.setBrush(g, r, palette.brush(g, r));
		}
	}
	return palette;
}

void qPrefDisplay::apply_color_scheme()
{
	// Without widgets (for instance a console or downloader application) there
	// is no appearance to change
	if (!qobject_cast<QApplication *>(QCoreApplication::instance()))
		return;

	// Remember the application palette from before the first override, so that
	// "Use system" restores exactly that instead of a hard-coded palette. A
	// palette without explicitly set roles is resolved against the platform
	// theme again, so the application follows the system as it did before.
	static std::optional<QPalette> systemPalette;
	if (!systemPalette)
		systemPalette = QApplication::palette();

	Qt::ColorScheme scheme = Qt::ColorScheme::Unknown;
	if (st_color_scheme == QLatin1String("light"))
		scheme = Qt::ColorScheme::Light;
	else if (st_color_scheme == QLatin1String("dark"))
		scheme = Qt::ColorScheme::Dark;

	QGuiApplication::styleHints()->setColorScheme(scheme);
	QApplication::setPalette(scheme == Qt::ColorScheme::Unknown ? *systemPalette : color_scheme_palette(scheme));
}

void qPrefDisplay::set_color_scheme(const QString &value)
{
	const QString newValue = validated_color_scheme(value);
	if (newValue != st_color_scheme) {
		st_color_scheme = newValue;
		qPrefPrivate::propSetValue(keyFromGroupAndName(group, "color_scheme"), st_color_scheme, st_color_scheme_default);
		apply_color_scheme();
		emit instance()->color_schemeChanged(st_color_scheme);
	}
}

void qPrefDisplay::load_color_scheme()
{
	st_color_scheme = validated_color_scheme(qPrefPrivate::propValue(keyFromGroupAndName(group, "color_scheme"), st_color_scheme_default).toString());
	apply_color_scheme();
}
#endif
