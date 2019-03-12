// SPDX-License-Identifier: GPL-2.0

#include "command_divesite.h"
#include "core/divesite.h"
#include "core/subsurface-qt/DiveListNotifier.h"

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

} // namespace Command
