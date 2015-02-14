#ifndef UNDOCOMMANDS_H
#define UNDOCOMMANDS_H

#include <QUndoCommand>
#include "dive.h"

class UndoDeleteDive : public QUndoCommand {
public:
	UndoDeleteDive(QList<struct dive*> diveList);
	virtual void undo();
	virtual void redo();

private:
	QList<struct dive*> dives;
};

class UndoShiftTime : public QUndoCommand {
public:
	UndoShiftTime(QList<int> diveList, int amount);
	virtual void undo();
	virtual void redo();

private:
	QList<int> dives;
	int timeChanged;
};

#endif // UNDOCOMMANDS_H
