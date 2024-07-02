// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFDISPLAY_H
#define QPREFDISPLAY_H
#include "core/pref.h"

#include <QObject>
#include <QPointF>

class qPrefDisplay : public QObject {
	Q_OBJECT
	Q_PROPERTY(int animation_speed READ animation_speed WRITE set_animation_speed NOTIFY animation_speedChanged)
	Q_PROPERTY(QString divelist_font READ divelist_font WRITE set_divelist_font NOTIFY divelist_fontChanged)
	Q_PROPERTY(double font_size READ font_size WRITE set_font_size NOTIFY font_sizeChanged)
	Q_PROPERTY(double mobile_scale READ mobile_scale WRITE set_mobile_scale NOTIFY mobile_scaleChanged)
	Q_PROPERTY(bool display_invalid_dives READ display_invalid_dives WRITE set_display_invalid_dives NOTIFY display_invalid_divesChanged)
	Q_PROPERTY(QString lastDir READ lastDir WRITE set_lastDir NOTIFY lastDirChanged)
	Q_PROPERTY(bool show_developer READ show_developer WRITE set_show_developer NOTIFY show_developerChanged)
	Q_PROPERTY(QString theme READ theme WRITE set_theme NOTIFY themeChanged)
	Q_PROPERTY(QPointF tooltip_position READ tooltip_position WRITE set_tooltip_position NOTIFY tooltip_positionChanged)
	Q_PROPERTY(QByteArray mainSplitter READ mainSplitter WRITE set_mainSplitter NOTIFY mainSplitterChanged)
	Q_PROPERTY(QByteArray topSplitter READ topSplitter WRITE set_topSplitter NOTIFY topSplitterChanged)
	Q_PROPERTY(QByteArray bottomSplitter READ bottomSplitter WRITE set_bottomSplitter NOTIFY bottomSplitterChanged)
	Q_PROPERTY(bool maximized READ maximized WRITE set_maximized NOTIFY maximizedChanged)
	Q_PROPERTY(QByteArray geometry READ geometry WRITE set_geometry NOTIFY geometryChanged)
	Q_PROPERTY(QByteArray windowState READ windowState WRITE set_windowState NOTIFY windowStateChanged)
	Q_PROPERTY(int lastState READ lastState WRITE set_lastState NOTIFY lastStateChanged)
	Q_PROPERTY(bool singleColumnPortrait READ singleColumnPortrait WRITE set_singleColumnPortrait NOTIFY singleColumnPortraitChanged)
	Q_PROPERTY(bool three_m_based_grid READ three_m_based_grid WRITE set_three_m_based_grid NOTIFY three_m_based_gridChanged)
	Q_PROPERTY(bool map_short_names READ map_short_names WRITE set_map_short_names NOTIFY map_short_namesChanged)

public:
	static qPrefDisplay *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static int animation_speed() { return prefs.animation_speed; }
	static QString divelist_font() { return QString::fromStdString(prefs.divelist_font); }
	static double font_size() { return prefs.font_size; }
	static double mobile_scale() { return prefs.mobile_scale; }
	static bool display_invalid_dives() { return prefs.display_invalid_dives; }
	static QString lastDir() { return st_lastDir; }
	static bool show_developer() { return prefs.show_developer; }
	static QString theme() { return st_theme; }
	static QPointF tooltip_position() { return st_tooltip_position; }
	static QByteArray mainSplitter() { return st_mainSplitter; }
	static QByteArray topSplitter() { return st_topSplitter; }
	static QByteArray bottomSplitter() { return st_bottomSplitter; }
	static bool maximized() { return st_maximized; }
	static QByteArray geometry() { return st_geometry; }
	static QByteArray windowState() { return st_windowState; }
	static int lastState() { return st_lastState; }
	static bool singleColumnPortrait() { return st_singleColumnPortrait; }
	static bool three_m_based_grid() { return prefs.three_m_based_grid; }
	static bool map_short_names() { return prefs.map_short_names; }

public slots:
	static void set_animation_speed(int value);
	static void set_divelist_font(const QString &value);
	static void set_font_size(double value);
	static void set_mobile_scale(double value);
	static void set_display_invalid_dives(bool value);
	static void set_lastDir(const QString &value);
	static void set_show_developer(bool value);
	static void set_theme(const QString &value);
	static void set_tooltip_position(const QPointF &value);
	static void set_mainSplitter(const QByteArray &value);
	static void set_topSplitter(const QByteArray &value);
	static void set_bottomSplitter(const QByteArray &value);
	static void set_maximized(bool value);
	static void set_geometry(const QByteArray& value);
	static void set_windowState(const QByteArray& value);
	static void set_lastState(int value);
	static void set_singleColumnPortrait(bool value);
	static void set_three_m_based_grid(bool value);
	static void set_map_short_names(bool value);

signals:
	void animation_speedChanged(int value);
	void divelist_fontChanged(const QString &value);
	void font_sizeChanged(double value);
	void mobile_scaleChanged(double value);
	void display_invalid_divesChanged(bool value);
	void lastDirChanged(const QString &value);
	void show_developerChanged(bool value);
	void themeChanged(const QString &value);
	void tooltip_positionChanged(const QPointF &value);
	void mainSplitterChanged(const QByteArray &value);
	void topSplitterChanged(const QByteArray &value);
	void bottomSplitterChanged(const QByteArray &value);
	void maximizedChanged(bool value);
	void geometryChanged(const QByteArray& value);
	void windowStateChanged(const QByteArray& value);
	void lastStateChanged(int value);
	void singleColumnPortraitChanged(bool value);
	void three_m_based_gridChanged(bool value);
	void map_short_namesChanged(bool value);

private:
	qPrefDisplay() {}

	// functions to load/sync variable with disk
	static void disk_animation_speed(bool doSync);
	static void disk_divelist_font(bool doSync);
	static void disk_font_size(bool doSync);
	static void disk_mobile_scale(bool doSync);
	static void disk_display_invalid_dives(bool doSync);
	static void disk_show_developer(bool doSync);
	static void disk_three_m_based_grid(bool doSync);
	static void disk_map_short_names(bool doSync);

	// functions to handle class variables
	static void load_lastDir();
	static void load_theme();
	static void load_tooltip_position();
	static void load_mainSplitter();
	static void load_topSplitter();
	static void load_bottomSplitter();
	static void load_maximized();
	static void load_geometry();
	static void load_windowState();
	static void load_lastState();
	static void load_singleColumnPortrait();

	// font helper function
	static void setCorrectFont();

	// Class variables not present in structure preferences
	static QString st_lastDir;
	static QString st_theme;
	static QPointF st_tooltip_position;
	static QByteArray st_mainSplitter;
	static QByteArray st_topSplitter;
	static QByteArray st_bottomSplitter;
	static bool st_maximized;
	static QByteArray st_geometry;
	static QByteArray st_windowState;
	static int st_lastState;
	static bool st_singleColumnPortrait;
};
#endif
