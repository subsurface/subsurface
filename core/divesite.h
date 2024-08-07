// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESITE_H
#define DIVESITE_H

#include "divelist.h"
#include "taxonomy.h"
#include "units.h"
#include <stdlib.h>

struct dive_site
{
	uint32_t uuid = 0;
	std::string name;
	std::vector<dive *> dives;
	location_t location;
	std::string description;
	std::string notes;
	taxonomy_data taxonomy;

	dive_site();
	dive_site(const std::string &name);
	dive_site(const std::string &name, const location_t loc);
	dive_site(uint32_t uuid);
	~dive_site();

	size_t nr_of_dives() const;
	bool is_selected() const;
	bool is_empty() const;
	bool has_gps_location() const;
	void merge(struct dive_site &b); // Note: b is consumed
	void add_dive(struct dive *d);
};

struct dive_site *unregister_dive_from_dive_site(struct dive *d);
int divesite_comp_uuid(const dive_site &ds1, const dive_site &ds2);

/* Make pointer-to-dive_site a "Qt metatype" so that we can pass it through QVariants */
#include <QObject>
Q_DECLARE_METATYPE(dive_site *);

#endif // DIVESITE_H
