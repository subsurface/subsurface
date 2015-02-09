#include "undobuffer.h"
#include "mainwindow.h"

UndoBuffer::UndoBuffer(QObject *parent) : QObject(parent)
{
	curIdx = 0;
}

UndoBuffer::~UndoBuffer()
{

}

bool UndoBuffer::canUndo()
{
	return curIdx > 0;
}

bool UndoBuffer::canRedo()
{
	return curIdx < list.count();
}

void UndoBuffer::redo()
{
	current()->redo();
	curIdx++;
	if (curIdx > list.count())
		curIdx = list.count() - 1;
}

void UndoBuffer::undo()
{
	current()->undo();
	curIdx = list.indexOf(current());
}

void UndoBuffer::recordbefore(QString commandName, dive *affectedDive)
{
	UndoCommand *cmd = new UndoCommand(commandName, affectedDive);
	//If we are within the list, clear the extra UndoCommands.
	if (list.count() > 0) {
		if (curIdx + 1 < list.count()) {
			for (int i = curIdx + 1; i < list.count(); i++) {
				list.removeAt(i);
			}
		}
	}
	list.append(cmd);
	curIdx = list.count();
}

void UndoBuffer::recordAfter(dive *affectedDive)
{
	list.at(curIdx - 1)->setStateAfter(affectedDive);
}



UndoCommand::UndoCommand(QString commandName, dive *affectedDive)
{
	name = commandName;
	stateBefore = affectedDive;
}

void UndoCommand::undo()
{
	if (name == "Delete Dive") {
		record_dive(stateBefore);
		MainWindow::instance()->recreateDiveList();
	}
}

void UndoCommand::redo()
{
	//To be implemented
}

