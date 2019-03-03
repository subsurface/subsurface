// SPDX-License-Identifier: GPL-2.0

#include "command.h"
#include "command_divelist.h"

namespace Command {

// Dive-list related commands
void addDive(dive *d, bool autogroup, bool newNumber)
{
	execute(new AddDive(d, autogroup, newNumber));
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

void mergeDives(const QVector <dive *> &dives)
{
	execute(new MergeDives(dives));
}

} // namespace Command
