#include "undocommands.h"
#include "mainwindow.h"
#include "divelist.h"

UndoDeleteDive::UndoDeleteDive(QList<dive *> deletedDives)
	: diveList(deletedDives)
{
	setText("delete dive");
	if (diveList.count() > 1)
		setText(QString("delete %1 dives").arg(QString::number(diveList.count())));
}

void UndoDeleteDive::undo()
{
	for (int i = 0; i < diveList.count(); i++)
		record_dive(diveList.at(i));
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
}

void UndoDeleteDive::redo()
{
	QList<struct dive*> newList;
	for (int i = 0; i < diveList.count(); i++) {
		//make a copy of the dive before deleting it
		struct dive* d = alloc_dive();
		copy_dive(diveList.at(i), d);
		newList.append(d);
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


UndoRenumberDives::UndoRenumberDives(QMap<int, int> originalNumbers, int startNumber)
{
	oldNumbers = originalNumbers;
	start = startNumber;
	setText("renumber dive");
	if (oldNumbers.count() > 1)
		setText(QString("renumber %1 dives").arg(QString::number(oldNumbers.count())));
}

void UndoRenumberDives::undo()
{
	foreach (int key, oldNumbers.keys()) {
		struct dive* d = get_dive_by_uniq_id(key);
		d->number = oldNumbers.value(key);
	}
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
}

void UndoRenumberDives::redo()
{
	int i = start;
	foreach (int key, oldNumbers.keys()) {
		struct dive* d = get_dive_by_uniq_id(key);
		d->number = i++;
	}
	mark_divelist_changed(true);
	MainWindow::instance()->refreshDisplay();
}
