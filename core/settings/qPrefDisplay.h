// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFDISPLAY_H
#define QPREFDISPLAY_H
#include "core/pref.h"

#include <QObject>

class qPrefDisplay : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString divelist_font READ divelist_font WRITE set_divelist_font NOTIFY divelist_font_changed);
	Q_PROPERTY(double font_size READ font_size WRITE set_font_size NOTIFY font_size_changed);
	Q_PROPERTY(bool display_invalid_dives READ display_invalid_dives WRITE set_display_invalid_dives NOTIFY display_invalid_dives_changed);
	Q_PROPERTY(bool show_developer READ show_developer WRITE set_show_developer NOTIFY show_developer_changed);
	Q_PROPERTY(QString theme READ theme WRITE set_theme NOTIFY theme_changed);

public:
	qPrefDisplay(QObject *parent = NULL);
	static qPrefDisplay *instance();

	// Load/Sync local settings (disk) and struct preference
	void loadSync(bool doSync);
	void load() { loadSync(false); }
	void sync() { loadSync(true); }

public:
	static QString divelist_font() { return prefs.divelist_font; }
	static double font_size() { return prefs.font_size; }
	static bool display_invalid_dives() { return prefs.display_invalid_dives; }
	static bool show_developer() { return prefs.show_developer; }
	static QString theme() { return prefs.theme; }

public slots:
	void set_divelist_font(const QString &value);
	void set_font_size(double value);
	void set_display_invalid_dives(bool value);
	void set_show_developer(bool value);
	void set_theme(const QString &value);

signals:
	void divelist_font_changed(const QString &value);
	void font_size_changed(double value);
	void display_invalid_dives_changed(bool value);
	void show_developer_changed(bool value);
	void theme_changed(const QString &value);

private:
	// functions to load/sync variable with disk
	void disk_divelist_font(bool doSync);
	void disk_font_size(bool doSync);
	void disk_display_invalid_dives(bool doSync);
	void disk_show_developer(bool doSync);
	void disk_theme(bool doSync);

	// font helper function
	void setCorrectFont();
};
#endif
