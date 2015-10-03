#include "undocommands.h"
#include "mainwindow.h"
#include "divelist.h"

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
			struct dive_trip *undo_trip = (struct dive_trip *)calloc(1, sizeof(struct dive_trip));
			*undo_trip = *d->divetrip;
			undo_trip->location = copy_string(d->divetrip->location);
			undo_trip->notes = copy_string(d->divetrip->notes);
			undo_trip->nrdives = 0;
			undo_trip->next = NULL;
			undo_trip->dives = NULL;
			// update all the dives who were in this trip to point to the copy of the
			// trip that we are about to delete implicitly when deleting its last dive below
			Q_FOREACH(struct dive *inner_dive, newList)
				if (inner_dive->divetrip == d->divetrip)
					inner_dive->divetrip = undo_trip;
			d->divetrip = undo_trip;
			tripList.append(undo_trip);
		}
		//delete the dive
		delete_single_dive(get_divenr(diveList.at(i)));
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
	QMapIterator<dive*, dive_trip*> i(divesToUndo);
	while (i.hasNext()) {
		i.next();
		add_dive_to_trip(i.key (), i.value());
	}
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
}

void UndoRemoveDivesFromTrip::redo()
{
	QMapIterator<dive*, dive_trip*> i(divesToUndo);
	while (i.hasNext()) {
		i.next();
		remove_dive_from_trip(i.key(), false);
	}
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
}
