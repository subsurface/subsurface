// SPDX-License-Identifier: GPL-2.0

#include "command.h"
#include "command_divelist.h"
#include "command_divesite.h"
#include "command_edit.h"

namespace Command {

// Dive-list related commands
void addDive(dive *d, const QString &newDS, bool autogroup, bool newNumber)
{
	execute(new AddDive(d, newDS, autogroup, newNumber));
}

void importDives(struct dive_table *dives, struct trip_table *trips, struct dive_site_table *sites, int flags, const QString &source)
{
	execute(new ImportDives(dives, trips, sites, flags, source));
}

void deleteDive(const QVector<struct dive*> &divesToDelete)
{
	execute(new DeleteDive(divesToDelete));
}

void shiftTime(const QVector<dive *> &changedDives, int amount)
{
	execute(new ShiftTime(changedDives, amount));
}

void renumberDives(const QVector<QPair<dive *, int>> &divesToRenumber)
{
	execute(new RenumberDives(divesToRenumber));
}

void removeDivesFromTrip(const QVector<dive *> &divesToRemove)
{
	execute(new RemoveDivesFromTrip(divesToRemove));
}

void removeAutogenTrips()
{
	execute(new RemoveAutogenTrips);
}

void addDivesToTrip(const QVector<dive *> &divesToAddIn, dive_trip *trip)
{
	execute(new AddDivesToTrip(divesToAddIn, trip));
}

void createTrip(const QVector<dive *> &divesToAddIn)
{
	execute(new CreateTrip(divesToAddIn));
}

void autogroupDives()
{
	execute(new AutogroupDives);
}

void mergeTrips(dive_trip *trip1, dive_trip *trip2)
{
	execute(new MergeTrips(trip1, trip2));
}

void splitDives(dive *d, duration_t time)
{
	execute(new SplitDives(d, time));
}

void splitDiveComputer(dive *d, int dc_num)
{
	execute(new SplitDiveComputer(d, dc_num));
}

void mergeDives(const QVector <dive *> &dives)
{
	execute(new MergeDives(dives));
}

// Dive site related commands
void deleteDiveSites(const QVector <dive_site *> &sites)
{
	execute(new DeleteDiveSites(sites));
}

void editDiveSiteName(dive_site *ds, const QString &value)
{
	execute(new EditDiveSiteName(ds, value));
}

void editDiveSiteDescription(dive_site *ds, const QString &value)
{
	execute(new EditDiveSiteDescription(ds, value));
}

void editDiveSiteNotes(dive_site *ds, const QString &value)
{
	execute(new EditDiveSiteNotes(ds, value));
}

void editDiveSiteCountry(dive_site *ds, const QString &value)
{
	execute(new EditDiveSiteCountry(ds, value));
}

void editDiveSiteLocation(dive_site *ds, location_t value)
{
	execute(new EditDiveSiteLocation(ds, value));
}

void editDiveSiteTaxonomy(dive_site *ds, taxonomy_data &value)
{
	execute(new EditDiveSiteTaxonomy(ds, value));
}

void addDiveSite(const QString &name)
{
	execute(new AddDiveSite(name));
}

void mergeDiveSites(dive_site *ds, const QVector<dive_site *> &sites)
{
	execute(new MergeDiveSites(ds, sites));
}

void purgeUnusedDiveSites()
{
	execute(new PurgeUnusedDiveSites);
}

// Dive editing related commands
void editNotes(const QVector<dive *> dives, const QString &newValue, const QString &oldValue)
{
	execute(new EditNotes(dives, newValue, oldValue));
}

void editMode(const QVector<dive *> dives, int index, int newValue, int oldValue)
{
	execute(new EditMode(dives, index, newValue, oldValue));
}

void editSuit(const QVector<dive *> dives, const QString &newValue, const QString &oldValue)
{
	execute(new EditSuit(dives, newValue, oldValue));
}

void editRating(const QVector<dive *> dives, int newValue, int oldValue)
{
	execute(new EditRating(dives, newValue, oldValue));
}

void editVisibility(const QVector<dive *> dives, int newValue, int oldValue)
{
	execute(new EditVisibility(dives, newValue, oldValue));
}

void editAirTemp(const QVector<dive *> dives, int newValue, int oldValue)
{
	execute(new EditAirTemp(dives, newValue, oldValue));
}

void editWaterTemp(const QVector<dive *> dives, int newValue, int oldValue)
{
	execute(new EditWaterTemp(dives, newValue, oldValue));
}

} // namespace Command
