#ifndef UNDOBUFFER_H
#define UNDOBUFFER_H

#include <QObject>
#include "dive.h"

class UndoCommand {
private:
	dive* stateBefore;
	dive* stateAfter;
	QString name;

public:
	explicit UndoCommand(QString commandName, dive* affectedDive);
	void setStateAfter(dive* affectedDive);
	void undo();
	void redo();
};

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
