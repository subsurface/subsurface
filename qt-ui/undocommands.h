#ifndef UNDOCOMMANDS_H
#define UNDOCOMMANDS_H

#include <QUndoCommand>
#include <QMap>
#include "dive.h"

class UndoDeleteDive : public QUndoCommand {
public:
	UndoDeleteDive(QList<struct dive*> deletedDives);
	virtual void undo();
	virtual void redo();

private:
	QList<struct dive*> diveList;
};

class UndoShiftTime : public QUndoCommand {
public:
	UndoShiftTime(QList<int> changedDives, int amount);
	virtual void undo();
	virtual void redo();

private:
	QList<int> diveList;
	int timeChanged;
};

class UndoRenumberDives : public QUndoCommand {
public:
	UndoRenumberDives(QMap<int, QPair<int, int> > originalNumbers);
	virtual void undo();
	virtual void redo();

private:
	QMap<int,QPair<int, int> > oldNumbers;
};

class UndoRemoveDivesFromTrip : public QUndoCommand {
public:
	UndoRemoveDivesFromTrip(QMap<struct dive*, dive_trip*> removedDives);
	virtual void undo();
	virtual void redo();

private:
	QMap<struct dive*, dive_trip*> divesToUndo;
};

#endif // UNDOCOMMANDS_H
