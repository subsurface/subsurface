// SPDX-License-Identifier: GPL-2.0
#ifndef QPREFGEOCODING_H
#define QPREFGEOCODING_H
#include "core/pref.h"

#include <QObject>

class qPrefGeocoding : public QObject {
	Q_OBJECT
	Q_PROPERTY(taxonomy_category first_taxonomy_category READ first_taxonomy_category WRITE set_first_taxonomy_category NOTIFY first_taxonomy_categoryChanged)
	Q_PROPERTY(taxonomy_category second_taxonomy_category READ second_taxonomy_category WRITE set_second_taxonomy_category NOTIFY second_taxonomy_categoryChanged)
	Q_PROPERTY(taxonomy_category third_taxonomy_category READ third_taxonomy_category WRITE set_third_taxonomy_category NOTIFY third_taxonomy_categoryChanged)

public:
	static qPrefGeocoding *instance();

	// Load/Sync local settings (disk) and struct preference
	static void loadSync(bool doSync);
	static void load() { loadSync(false); }
	static void sync() { loadSync(true); }

public:
	static taxonomy_category first_taxonomy_category() { return prefs.geocoding.category[0]; }
	static taxonomy_category second_taxonomy_category() { return prefs.geocoding.category[1]; }
	static taxonomy_category third_taxonomy_category() { return prefs.geocoding.category[2]; }

public slots:
	static void set_first_taxonomy_category(taxonomy_category value);
	static void set_second_taxonomy_category(taxonomy_category value);
	static void set_third_taxonomy_category(taxonomy_category value);

signals:
	void first_taxonomy_categoryChanged(taxonomy_category value);
	void second_taxonomy_categoryChanged(taxonomy_category value);
	void third_taxonomy_categoryChanged(taxonomy_category value);

private:
	qPrefGeocoding() {}

	static void disk_first_taxonomy_category(bool doSync);
	static void disk_second_taxonomy_category(bool doSync);
	static void disk_third_taxonomy_category(bool doSync);
};

#endif
