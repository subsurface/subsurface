#ifndef UNDOBUFFER_H
#define UNDOBUFFER_H

#include <QObject>
#include "dive.h"

class UndoBuffer : public QObject
{
	Q_OBJECT
public:
	explicit UndoBuffer(QObject *parent = 0);
	~UndoBuffer();
	bool canUndo();
	bool canRedo();
private:
	int curIdx;
public slots:
	void redo();
	void undo();
	void recordbefore(QString commandName, dive *affectedDive);
	void recordAfter(dive *affectedDive);
};

#endif // UNDOBUFFER_H
