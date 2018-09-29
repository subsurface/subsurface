// SPDX-License-Identifier: GPL-2.0
#ifndef UNDOCOMMANDS_H
#define UNDOCOMMANDS_H

#include <QUndoCommand>
#include <QMap>

class UndoDeleteDive : public QUndoCommand {
public:
	UndoDeleteDive(QList<struct dive*> deletedDives);
	void undo() override;
	void redo() override;

private:
	QList<struct dive*> diveList;
	QList<struct dive_trip*> tripList;
};

class UndoShiftTime : public QUndoCommand {
public:
	UndoShiftTime(QList<int> changedDives, int amount);
	void undo() override;
	void redo() override;

private:
	QList<int> diveList;
	int timeChanged;
};

class UndoRenumberDives : public QUndoCommand {
public:
	UndoRenumberDives(QMap<int, QPair<int, int> > originalNumbers);
	void undo() override;
	void redo() override;

private:
	QMap<int,QPair<int, int> > oldNumbers;
};

class UndoRemoveDivesFromTrip : public QUndoCommand {
public:
	UndoRemoveDivesFromTrip(QMap<struct dive*, dive_trip*> removedDives);
	void undo() override;
	void redo() override;

private:
	QMap<struct dive*, dive_trip*> divesToUndo;
	QList<struct dive_trip*> tripList;
};

#endif // UNDOCOMMANDS_H
