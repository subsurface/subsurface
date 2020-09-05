// SPDX-License-Identifier: GPL-2.0

#include "command_divesite.h"
#include "core/divesite.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include "qt-models/divelocationmodel.h"
#include "qt-models/filtermodels.h"

namespace Command {

// Helper functions to add / remove a set of dive sites

// Add a set of dive sites to the core. The dives that were associated with
// that dive site will be restored to that dive site.
static std::vector<dive_site *> addDiveSites(std::vector<OwningDiveSitePtr> &sites)
{
	std::vector<dive_site *> res;
	QVector<dive *> changedDives;
	res.reserve(sites.size());

	for (OwningDiveSitePtr &ds: sites) {
		// Readd the dives that belonged to this site
		for (int i = 0; i < ds->dives.nr; ++i) {
			// TODO: send dive site changed signal
			struct dive *d = ds->dives.dives[i];
			d->dive_site = ds.get();
			changedDives.push_back(d);
		}

		// Add dive site to core, but remember a non-owning pointer first.
		res.push_back(ds.get());
		int idx = register_dive_site(ds.release()); // Return ownership to backend.
		emit diveListNotifier.diveSiteAdded(res.back(), idx); // Inform frontend of new dive site.
	}

	emit diveListNotifier.divesChanged(changedDives, DiveField::DIVESITE);

	// Clear vector of unused owning pointers
	sites.clear();

	return res;
}

// Remove a set of dive sites. Get owning pointers to them. The dives are set to
// being at no dive site, but the dive site will retain a list of dives, so
// that the dives can be readded to the site on undo.
static std::vector<OwningDiveSitePtr> removeDiveSites(std::vector<dive_site *> &sites)
{
	std::vector<OwningDiveSitePtr> res;
	QVector<dive *> changedDives;
	res.reserve(sites.size());

	for (dive_site *ds: sites) {
		// Reset the dive_site field of the affected dives
		for (int i = 0; i < ds->dives.nr; ++i) {
			struct dive *d = ds->dives.dives[i];
			d->dive_site = nullptr;
			changedDives.push_back(d);
		}

		// Remove dive site from core and take ownership.
		int idx = unregister_dive_site(ds);
		res.emplace_back(ds);
		emit diveListNotifier.diveSiteDeleted(ds, idx); // Inform frontend of removed dive site.
	}

	emit diveListNotifier.divesChanged(changedDives, DiveField::DIVESITE);

	sites.clear();

	return res;
}

AddDiveSite::AddDiveSite(const QString &name)
{
	setText(Command::Base::tr("add dive site"));
	sitesToAdd.emplace_back(alloc_dive_site());
	sitesToAdd.back()->name = copy_qstring(name);
}

bool AddDiveSite::workToBeDone()
{
	return true;
}

void AddDiveSite::redo()
{
	sitesToRemove = addDiveSites(sitesToAdd);
}

void AddDiveSite::undo()
{
	sitesToAdd = removeDiveSites(sitesToRemove);
}

ImportDiveSites::ImportDiveSites(struct dive_site_table *sites, const QString &source)
{
	setText(Command::Base::tr("import dive sites from %1").arg(source));

	for (int i = 0; i < sites->nr; ++i) {
		struct dive_site *new_ds = sites->dive_sites[i];

		// Don't import dive sites that already exist. Currently we only check for
		// the same name. We might want to be smarter here and merge dive site data, etc.
		struct dive_site *old_ds = get_same_dive_site(new_ds);
		if (old_ds) {
			free_dive_site(new_ds);
			continue;
		}
		sitesToAdd.emplace_back(new_ds);
	}

	// All site have been consumed
	sites->nr = 0;
}

bool ImportDiveSites::workToBeDone()
{
	return !sitesToAdd.empty();
}

void ImportDiveSites::redo()
{
	sitesToRemove = addDiveSites(sitesToAdd);
}

void ImportDiveSites::undo()
{
	sitesToAdd = removeDiveSites(sitesToRemove);
}

DeleteDiveSites::DeleteDiveSites(const QVector<dive_site *> &sites) : sitesToRemove(std::vector<dive_site *>(sites.begin(),sites.end()))
{
	setText(Command::Base::tr("delete %n dive site(s)", "", sites.size()));
}

bool DeleteDiveSites::workToBeDone()
{
	return !sitesToRemove.empty();
}

void DeleteDiveSites::redo()
{
	sitesToAdd = removeDiveSites(sitesToRemove);
}

void DeleteDiveSites::undo()
{
	sitesToRemove = addDiveSites(sitesToAdd);
}

PurgeUnusedDiveSites::PurgeUnusedDiveSites()
{
	setText(Command::Base::tr("purge unused dive sites"));
	for (int i = 0; i < dive_site_table.nr; ++i) {
		dive_site *ds = dive_site_table.dive_sites[i];
		if (ds->dives.nr == 0)
			sitesToRemove.push_back(ds);
	}
}

bool PurgeUnusedDiveSites::workToBeDone()
{
	return !sitesToRemove.empty();
}

void PurgeUnusedDiveSites::redo()
{
	sitesToAdd = removeDiveSites(sitesToRemove);
}

void PurgeUnusedDiveSites::undo()
{
	sitesToRemove = addDiveSites(sitesToAdd);
}

// Helper function: swap C and Qt string
static void swap(char *&c, QString &q)
{
	QString s = c;
	free(c);
	c = copy_qstring(q);
	q = s;
}

EditDiveSiteName::EditDiveSiteName(dive_site *dsIn, const QString &name) : ds(dsIn),
	value(name)
{
	setText(Command::Base::tr("Edit dive site name"));
}

bool EditDiveSiteName::workToBeDone()
{
	return value != QString(ds->name);
}

void EditDiveSiteName::redo()
{
	swap(ds->name, value);
	emit diveListNotifier.diveSiteChanged(ds, LocationInformationModel::NAME); // Inform frontend of changed dive site.
}

void EditDiveSiteName::undo()
{
	// Undo and redo do the same
	redo();
}

EditDiveSiteDescription::EditDiveSiteDescription(dive_site *dsIn, const QString &description) : ds(dsIn),
	value(description)
{
	setText(Command::Base::tr("Edit dive site description"));
}

bool EditDiveSiteDescription::workToBeDone()
{
	return value != QString(ds->description);
}

void EditDiveSiteDescription::redo()
{
	swap(ds->description, value);
	emit diveListNotifier.diveSiteChanged(ds, LocationInformationModel::DESCRIPTION); // Inform frontend of changed dive site.
}

void EditDiveSiteDescription::undo()
{
	// Undo and redo do the same
	redo();
}

EditDiveSiteNotes::EditDiveSiteNotes(dive_site *dsIn, const QString &notes) : ds(dsIn),
	value(notes)
{
	setText(Command::Base::tr("Edit dive site notes"));
}

bool EditDiveSiteNotes::workToBeDone()
{
	return value != QString(ds->notes);
}

void EditDiveSiteNotes::redo()
{
	swap(ds->notes, value);
	emit diveListNotifier.diveSiteChanged(ds, LocationInformationModel::NOTES); // Inform frontend of changed dive site.
}

void EditDiveSiteNotes::undo()
{
	// Undo and redo do the same
	redo();
}

EditDiveSiteCountry::EditDiveSiteCountry(dive_site *dsIn, const QString &country) : ds(dsIn),
	value(country)
{
	setText(Command::Base::tr("Edit dive site country"));
}

bool EditDiveSiteCountry::workToBeDone()
{
	return !same_string(qPrintable(value), taxonomy_get_country(&ds->taxonomy));
}

void EditDiveSiteCountry::redo()
{
	QString old = taxonomy_get_country(&ds->taxonomy);
	taxonomy_set_country(&ds->taxonomy, qPrintable(value), taxonomy_origin::GEOMANUAL);
	value = old;
	emit diveListNotifier.diveSiteChanged(ds, LocationInformationModel::TAXONOMY); // Inform frontend of changed dive site.
}

void EditDiveSiteCountry::undo()
{
	// Undo and redo do the same
	redo();
}

EditDiveSiteLocation::EditDiveSiteLocation(dive_site *dsIn, const location_t location) : ds(dsIn),
	value(location)
{
	setText(Command::Base::tr("Edit dive site location"));
}

bool EditDiveSiteLocation::workToBeDone()
{
	bool ok = has_location(&value);
	bool old_ok = has_location(&ds->location);
	if (ok != old_ok)
		return true;
	return ok && !same_location(&value, &ds->location);
}

void EditDiveSiteLocation::redo()
{
	std::swap(value, ds->location);
	emit diveListNotifier.diveSiteChanged(ds, LocationInformationModel::LOCATION); // Inform frontend of changed dive site.
}

void EditDiveSiteLocation::undo()
{
	// Undo and redo do the same
	redo();
}

EditDiveSiteTaxonomy::EditDiveSiteTaxonomy(dive_site *dsIn, taxonomy_data &taxonomy) : ds(dsIn),
	value(taxonomy)
{
	// We did a dumb copy. Erase the source to remove double references to strings.
	memset(&taxonomy, 0, sizeof(taxonomy));
	setText(Command::Base::tr("Edit dive site taxonomy"));
}

EditDiveSiteTaxonomy::~EditDiveSiteTaxonomy()
{
	free_taxonomy(&value);
}

bool EditDiveSiteTaxonomy::workToBeDone()
{
	// TODO: Apparently we have no way of comparing taxonomies?
	return true;
}

void EditDiveSiteTaxonomy::redo()
{
	std::swap(value, ds->taxonomy);
	emit diveListNotifier.diveSiteChanged(ds, LocationInformationModel::TAXONOMY); // Inform frontend of changed dive site.
}

void EditDiveSiteTaxonomy::undo()
{
	// Undo and redo do the same
	redo();
}

MergeDiveSites::MergeDiveSites(dive_site *dsIn, const QVector<dive_site *> &sites) : ds(dsIn)
{
	setText(Command::Base::tr("merge dive sites"));
	sitesToRemove.reserve(sites.size());
	for (dive_site *site: sites) {
		if (site != ds)
			sitesToRemove.push_back(site);
	}
}

bool MergeDiveSites::workToBeDone()
{
	return !sitesToRemove.empty();
}

void MergeDiveSites::redo()
{
	// First, remove all dive sites
	sitesToAdd = removeDiveSites(sitesToRemove);

	// Remember which dives changed so that we can send a single dives-edited signal
	QVector<dive *> divesChanged;

	// The dives of the above dive sites were reset to no dive sites.
	// Add them to the merged-into dive site. Thankfully, we remember
	// the dives in the sitesToAdd vector.
	for (const OwningDiveSitePtr &site: sitesToAdd) {
		for (int i = 0; i < site->dives.nr; ++i) {
			add_dive_to_dive_site(site->dives.dives[i], ds);
			divesChanged.push_back(site->dives.dives[i]);
		}
	}
	emit diveListNotifier.divesChanged(divesChanged, DiveField::DIVESITE);
}

void MergeDiveSites::undo()
{
	// Remember which dives changed so that we can send a single dives-edited signal
	QVector<dive *> divesChanged;

	// Before readding the dive sites, unregister the corresponding dives so that they can be
	// readded to their old dive sites.
	for (const OwningDiveSitePtr &site: sitesToAdd) {
		for (int i = 0; i < site->dives.nr; ++i) {
			unregister_dive_from_dive_site(site->dives.dives[i]);
			divesChanged.push_back(site->dives.dives[i]);
		}
	}

	sitesToRemove = addDiveSites(sitesToAdd);

	emit diveListNotifier.divesChanged(divesChanged, DiveField::DIVESITE);
}

ApplyGPSFixes::ApplyGPSFixes(const std::vector<DiveAndLocation> &fixes)
{
	setText(Command::Base::tr("apply GPS fixes"));

	for (const DiveAndLocation &dl: fixes) {
		struct dive_site *ds = dl.d->dive_site;
		if (ds) {
			// Arbitrary choice: if we find multiple fixes for the same dive, we use the first one.
			if (std::find_if(siteLocations.begin(), siteLocations.end(),
					 [ds] (const SiteAndLocation &sl) { return sl.ds == ds; }) == siteLocations.end()) {
				siteLocations.push_back({ ds, dl.location });
			}
		} else {
			ds = create_dive_site(qPrintable(dl.name), &dive_site_table);
			ds->location = dl.location;
			add_dive_to_dive_site(dl.d, ds);
			dl.d->dive_site = nullptr; // This will be set on redo()
			sitesToAdd.emplace_back(ds);
		}
	}
}

bool ApplyGPSFixes::workToBeDone()
{
	return !sitesToAdd.empty() || !siteLocations.empty();
}

void ApplyGPSFixes::editDiveSites()
{
	for (SiteAndLocation &sl: siteLocations) {
		std::swap(sl.location, sl.ds->location);
		emit diveListNotifier.diveSiteChanged(sl.ds, LocationInformationModel::LOCATION); // Inform frontend of changed dive site.
	}
}

void ApplyGPSFixes::redo()
{
	sitesToRemove = addDiveSites(sitesToAdd);
	editDiveSites();
}

void ApplyGPSFixes::undo()
{
	sitesToAdd = removeDiveSites(sitesToRemove);
	editDiveSites();
}

} // namespace Command
