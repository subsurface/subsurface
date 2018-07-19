// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/undocommands.h"
#include "desktop-widgets/mainwindow.h"
#include "core/divelist.h"
#include "core/subsurface-string.h"
#include "core/gettextfromc.h"

UndoDeleteDive::UndoDeleteDive(const QVector<struct dive*> &divesToDeleteIn) : divesToDelete(divesToDeleteIn)
{
	setText(tr("delete %n dive(s)", "", divesToDelete.size()));
}

void UndoDeleteDive::undo()
{
	// first bring back the trip(s)
	for (auto &trip: tripsToAdd) {
		dive_trip *t = trip.release();	// Give up ownership
		insert_trip(&t);		// Return ownership to backend
	}
	tripsToAdd.clear();

	for (DiveToAdd &d: divesToAdd) {
		if (d.trip)
			add_dive_to_trip(d.dive.get(), d.trip);
		divesToDelete.append(d.dive.get());		// Delete dive on redo
		add_single_dive(d.idx, d.dive.release());	// Return ownership to backend
	}
	mark_divelist_changed(true);
	divesToAdd.clear();

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}

void UndoDeleteDive::redo()
{
	for (dive *d: divesToDelete) {
		int idx = get_divenr(d);
		if (idx < 0) {
			qWarning() << "Deletion of unknown dive!";
			continue;
		}
		// remove dive from trip - if this is the last dive in the trip
		// remove the whole trip.
		dive_trip *trip = unregister_dive_from_trip(d, false);
		if (trip && trip->nrdives == 0) {
			unregister_trip(trip);			// Remove trip from backend
			tripsToAdd.emplace_back(trip);		// Take ownership of trip
		}

		unregister_dive(idx);			// Remove dive from backend
		divesToAdd.push_back({ OwningDivePtr(d), trip, idx });
							// Take ownership for dive
	}
	divesToDelete.clear();
	mark_divelist_changed(true);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}


UndoShiftTime::UndoShiftTime(QVector<int> changedDives, int amount)
	: diveList(changedDives), timeChanged(amount)
{
	setText(tr("delete %n dive(s)", "", changedDives.size()));
}

void UndoShiftTime::undo()
{
	for (int i = 0; i < diveList.count(); i++) {
		dive *d = get_dive_by_uniq_id(diveList.at(i));
		d->when -= timeChanged;
	}
	// Changing times may have unsorted the dive table
	sort_table(&dive_table);
	mark_divelist_changed(true);

	// Negate the time-shift so that the next call does the reverse
	timeChanged = -timeChanged;

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}

void UndoShiftTime::redo()
{
	// Same as undo(), since after undo() we reversed the timeOffset
	undo();
}


UndoRenumberDives::UndoRenumberDives(const QVector<QPair<int, int>> &divesToRenumberIn) : divesToRenumber(divesToRenumberIn)
{
	setText(tr("renumber %n dive(s)", "", divesToRenumber.count()));
}

void UndoRenumberDives::undo()
{
	for (auto &pair: divesToRenumber) {
		dive *d = get_dive_by_uniq_id(pair.first);
		if (!d)
			continue;
		std::swap(d->number, pair.second);
	}
	mark_divelist_changed(true);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}

void UndoRenumberDives::redo()
{
	// Redo and undo do the same thing!
	undo();
}

UndoRemoveDivesFromTrip::UndoRemoveDivesFromTrip(const QVector<dive *> &divesToRemoveIn) : divesToRemove(divesToRemoveIn)
{
	setText(tr("remove %n dive(s) from trip", "", divesToRemove.size()));
}

void UndoRemoveDivesFromTrip::undo()
{
	// first bring back the trip(s)
	for (auto &trip: tripsToAdd) {
		dive_trip *t = trip.release();	// Give up ownership
		insert_trip(&t);		// Return ownership to backend
	}
	tripsToAdd.clear();

	for (auto &pair: divesToAdd)
		add_dive_to_trip(pair.first, pair.second);
	divesToAdd.clear();

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}

void UndoRemoveDivesFromTrip::redo()
{
	for (dive *d: divesToRemove) {
		// remove dive from trip - if this is the last dive in the trip
		// remove the whole trip.
		dive_trip *trip = unregister_dive_from_trip(d, false);
		if (!trip)
			continue;				// This was not part of a trip
		if (trip->nrdives == 0) {
			unregister_trip(trip);			// Remove trip from backend
			tripsToAdd.emplace_back(trip);		// Take ownership of trip
		}
		divesToAdd.emplace_back(d, trip);
	}
	mark_divelist_changed(true);

	// Finally, do the UI stuff:
	MainWindow::instance()->refreshDisplay();
}
