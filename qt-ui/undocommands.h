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

#endif // UNDOCOMMANDS_H
