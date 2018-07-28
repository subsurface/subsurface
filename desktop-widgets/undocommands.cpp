// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/undocommands.h"
#include "desktop-widgets/mainwindow.h"
#include "core/divelist.h"
#include "core/subsurface-string.h"

UndoDeleteDive::UndoDeleteDive(QList<dive *> deletedDives) : diveList(deletedDives)
{
	setText("delete dive");
	if (diveList.count() > 1)
		setText(QString("delete %1 dives").arg(QString::number(diveList.count())));
}

void UndoDeleteDive::undo()
{
	// first bring back the trip(s)
	Q_FOREACH(struct dive_trip *trip, tripList)
		insert_trip(&trip);

	// now walk the list of deleted dives
	for (int i = 0; i < diveList.count(); i++) {
		struct dive *d = diveList.at(i);
		// we adjusted the divetrip to point to the "new" divetrip
		if (d->divetrip) {
			struct dive_trip *trip = d->divetrip;
			tripflag_t tripflag = d->tripflag; // this gets overwritten in add_dive_to_trip()
			d->divetrip = NULL;
			d->next = NULL;
			d->pprev = NULL;
			add_dive_to_trip(d, trip);
			d->tripflag = tripflag;
		}
		record_dive(diveList.at(i));
	}
	mark_divelist_changed(true);
	tripList.clear();
	MainWindow::instance()->refreshDisplay();
}

void UndoDeleteDive::redo()
{
	QList<struct dive*> newList;
	for (int i = 0; i < diveList.count(); i++) {
		// make a copy of the dive before deleting it
		struct dive* d = alloc_dive();
		copy_dive(diveList.at(i), d);
		newList.append(d);
		// check for trip - if this is the last dive in the trip
		// the trip will get deleted, so we need to remember it as well
		if (d->divetrip && d->divetrip->nrdives == 1) {
			dive_trip *undo_trip = clone_empty_trip(d->divetrip);
			// update all the dives who were in this trip to point to the copy of the
			// trip that we are about to delete implicitly when deleting its last dive below
			Q_FOREACH(struct dive *inner_dive, newList) {
				if (inner_dive->divetrip == d->divetrip)
					inner_dive->divetrip = undo_trip;
			}
			d->divetrip = undo_trip;
			tripList.append(undo_trip);
		}
		//delete the dive
		int nr;
		if ((nr = get_divenr(diveList.at(i))) >= 0)
			delete_single_dive(nr);
	}
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
	diveList.clear();
	diveList = newList;
}


UndoShiftTime::UndoShiftTime(QList<int> changedDives, int amount)
	: diveList(changedDives), timeChanged(amount)
{
	setText("shift time");
}

void UndoShiftTime::undo()
{
	for (int i = 0; i < diveList.count(); i++) {
		struct dive* d = get_dive_by_uniq_id(diveList.at(i));
		d->when -= timeChanged;
	}
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
}

void UndoShiftTime::redo()
{
	for (int i = 0; i < diveList.count(); i++) {
		struct dive* d = get_dive_by_uniq_id(diveList.at(i));
		d->when += timeChanged;
	}
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
}


UndoRenumberDives::UndoRenumberDives(QMap<int, QPair<int, int> > originalNumbers)
{
	oldNumbers = originalNumbers;
	if (oldNumbers.count() > 1)
		setText(QString("renumber %1 dives").arg(QString::number(oldNumbers.count())));
	else
		setText("renumber dive");
}

void UndoRenumberDives::undo()
{
	foreach (int key, oldNumbers.keys()) {
		struct dive* d = get_dive_by_uniq_id(key);
		d->number = oldNumbers.value(key).first;
	}
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
}

void UndoRenumberDives::redo()
{
	foreach (int key, oldNumbers.keys()) {
		struct dive* d = get_dive_by_uniq_id(key);
		d->number = oldNumbers.value(key).second;
	}
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
}


UndoRemoveDivesFromTrip::UndoRemoveDivesFromTrip(QMap<dive *, dive_trip *> removedDives)
{
	divesToUndo = removedDives;
	setText("remove dive(s) from trip");
}

void UndoRemoveDivesFromTrip::undo()
{
	// first bring back the trip(s)
	Q_FOREACH(struct dive_trip *trip, tripList)
		insert_trip(&trip);
	tripList.clear();

	QMapIterator<dive*, dive_trip*> i(divesToUndo);
	while (i.hasNext()) {
		i.next();
		add_dive_to_trip(i.key(), i.value());
	}
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
}

void UndoRemoveDivesFromTrip::redo()
{
	QMapIterator<dive*, dive_trip*> i(divesToUndo);
	while (i.hasNext()) {
		i.next();
		// If the trip will be deleted, remember it so that we can restore it later.
		dive_trip *trip = i.value();
		if (trip->nrdives == 1) {
			dive_trip *cloned_trip = clone_empty_trip(trip);
			tripList.append(cloned_trip);
			// Rewrite the dive list, such that the dives will be added to the resurrected trip.
			for (dive_trip *&old_trip: divesToUndo) {
				if (old_trip == trip)
					old_trip = cloned_trip;
			}
		}
		remove_dive_from_trip(i.key(), false);
	}
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
}
