// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFLOG_H
#define QPREFLOG_H
#include "core/pref.h"

#include <QObject>

class qPrefLog : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString default_filename READ default_filename WRITE set_default_filename NOTIFY default_filenameChanged)
	Q_PROPERTY(enum def_file_behavior default_file_behavior READ default_file_behavior WRITE set_default_file_behavior NOTIFY default_file_behaviorChanged)
	Q_PROPERTY(bool use_default_file READ use_default_file WRITE set_use_default_file NOTIFY use_default_fileChanged)
	Q_PROPERTY(bool extraEnvironmentalDefault READ extraEnvironmentalDefault WRITE set_extraEnvironmentalDefault NOTIFY extraEnvironmentalDefaultChanged);
	Q_PROPERTY(bool salinityEditDefault READ salinityEditDefault WRITE set_salinityEditDefault NOTIFY salinityEditDefaultChanged);
	Q_PROPERTY(bool show_average_depth READ show_average_depth WRITE set_show_average_depth NOTIFY show_average_depthChanged)

public:
	static qPrefLog *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { return loadSync(false); }
	static void sync() { return loadSync(true); }

public:
	static QString default_filename() { return QString::fromStdString(prefs.default_filename); }
	static enum def_file_behavior default_file_behavior() { return prefs.default_file_behavior; }
	static bool use_default_file() { return prefs.use_default_file; }
	static bool extraEnvironmentalDefault() { return prefs.extraEnvironmentalDefault; }
	static bool salinityEditDefault() { return prefs.salinityEditDefault; }
	static bool show_average_depth() { return prefs.show_average_depth; }

public slots:
	static void set_default_filename(const QString& value);
	static void set_default_file_behavior(enum def_file_behavior value);
	static void set_use_default_file(bool value);
	static void set_extraEnvironmentalDefault(bool value);
	static void set_salinityEditDefault(bool value);
	static void set_show_average_depth(bool value);

signals:
	void default_filenameChanged(const QString& value);
	void default_file_behaviorChanged(enum def_file_behavior value);
	void use_default_fileChanged(bool value);
	void extraEnvironmentalDefaultChanged(bool value);
	void salinityEditDefaultChanged(bool value);
	void show_average_depthChanged(bool value);

private:
	qPrefLog() {}

	static void disk_default_filename(bool doSync);
	static void disk_default_file_behavior(bool doSync);
	static void disk_use_default_file(bool doSync);
	static void disk_extraEnvironmentalDefault(bool doSync);
	static void disk_salinityEditDefault(bool doSync);
	static void disk_show_average_depth(bool doSync);

};

#endif
