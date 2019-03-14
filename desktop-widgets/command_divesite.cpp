// SPDX-License-Identifier: GPL-2.0

#include "command_divesite.h"
#include "core/divesite.h"
#include "core/subsurface-qt/DiveListNotifier.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include "qt-models/divelocationmodel.h"

namespace Command {

// Helper functions to add / remove a set of dive sites

// Add a set of dive sites to the core. The dives that were associated with
// that dive site will be restored to that dive site.
static std::vector<dive_site *> addDiveSites(std::vector<OwningDiveSitePtr> &sites)
{
	std::vector<dive_site *> res;
	res.reserve(sites.size());

	for (OwningDiveSitePtr &ds: sites) {
		// Readd the dives that belonged to this site
		for (int i = 0; i < ds->dives.nr; ++i) {
			// TODO: send dive site changed signal
			ds->dives.dives[i]->dive_site = ds.get();
		}

		// Add dive site to core, but remember a non-owning pointer first.
		res.push_back(ds.get());
		int idx = register_dive_site(ds.release()); // Return ownership to backend.
		emit diveListNotifier.diveSiteAdded(res.back(), idx); // Inform frontend of new dive site.
	}

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
	res.reserve(sites.size());

	for (dive_site *ds: sites) {
		// Reset the dive_site field of the affected dives
		for (int i = 0; i < ds->dives.nr; ++i) {
			// TODO: send dive site changed signal
			ds->dives.dives[i]->dive_site = nullptr;
		}

		// Remove dive site from core and take ownership.
		int idx = unregister_dive_site(ds);
		res.emplace_back(ds);
		emit diveListNotifier.diveSiteDeleted(ds, idx); // Inform frontend of removed dive site.
	}

	sites.clear();

	return res;
}

AddDiveSite::AddDiveSite(const QString &name)
{
	setText(tr("add dive site"));
	sitesToAdd.emplace_back(alloc_dive_site());
	sitesToAdd.back()->name = copy_qstring(name);
}

bool AddDiveSite::workToBeDone()
{
	return true;
}

void AddDiveSite::redo()
{
	sitesToRemove = std::move(addDiveSites(sitesToAdd));
}

void AddDiveSite::undo()
{
	sitesToAdd = std::move(removeDiveSites(sitesToRemove));
}

DeleteDiveSites::DeleteDiveSites(const QVector<dive_site *> &sites) : sitesToRemove(sites.toStdVector())
{
	setText(tr("delete %n dive site(s)", "", sites.size()));
}

bool DeleteDiveSites::workToBeDone()
{
	return !sitesToRemove.empty();
}

void DeleteDiveSites::redo()
{
	sitesToAdd = std::move(removeDiveSites(sitesToRemove));
}

void DeleteDiveSites::undo()
{
	sitesToRemove = std::move(addDiveSites(sitesToAdd));
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
	setText(tr("Edit dive site name"));
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
	setText(tr("Edit dive site description"));
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
	setText(tr("Edit dive site notes"));
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
	setText(tr("Edit dive site country"));
}

bool EditDiveSiteCountry::workToBeDone()
{
	return !same_string(qPrintable(value), taxonomy_get_country(&ds->taxonomy));
}

void EditDiveSiteCountry::redo()
{
	QString old = taxonomy_get_country(&ds->taxonomy);
	taxonomy_set_country(&ds->taxonomy, copy_qstring(value), taxonomy_origin::GEOMANUAL);
	value = old;
	emit diveListNotifier.diveSiteChanged(ds, LocationInformationModel::TAXONOMY); // Inform frontend of changed dive site.
}

void EditDiveSiteCountry::undo()
{
	// Undo and redo do the same
	redo();
}

// Parse GPS text into location_t
static location_t parseGpsText(const QString &text)
{
	double lat, lon;
	if (parseGpsText(text, &lat, &lon))
		return create_location(lat, lon);
	return { {0}, {0} };
}

EditDiveSiteLocation::EditDiveSiteLocation(dive_site *dsIn, const QString &location) : ds(dsIn),
	value(parseGpsText(location))
{
	setText(tr("Edit dive site location"));
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

} // namespace Command
