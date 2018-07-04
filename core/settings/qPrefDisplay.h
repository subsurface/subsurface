// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFDISPLAY_H
#define QPREFDISPLAY_H

#include <QObject>
#include <QSettings>

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
	const QString divelist_font() const;
	double font_size() const;
	bool display_invalid_dives() const;
	bool show_developer() const;
	const QString theme() const;

public slots:
	void set_divelist_font(const QString& value);
	void set_font_size(double value);
	void set_display_invalid_dives(bool value);
	void set_show_developer(bool value);
	void set_theme(const QString& value);

signals:
	void divelist_font_changed(const QString& value);
	void font_size_changed(double value);
	void display_invalid_dives_changed(bool value);
	void show_developer_changed(bool value);
	void theme_changed(const QString& value);

private:
	const QString group = QStringLiteral("Display");
	QSettings setting;

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
