// SPDX-License-Identifier: GPL-2.0

#include "command_base.h"
#include "core/qthelper.h" // for updateWindowTitle()
#include "core/subsurface-qt/divelistnotifier.h"

namespace Command {

static QUndoStack undoStack;

// General commands
void init()
{
	QObject::connect(&undoStack, &QUndoStack::cleanChanged, &updateWindowTitle);
}

void clear()
{
	undoStack.clear();
}

void setClean()
{
	undoStack.setClean();
}

bool isClean()
{
	return undoStack.isClean();
}

// this can be used to get access to the signals emitted by the QUndoStack
QUndoStack *getUndoStack()
{
	return &undoStack;
}

QAction *undoAction(QObject *parent)
{
	return undoStack.createUndoAction(parent, QCoreApplication::translate("Command", "&Undo"));
}

QAction *redoAction(QObject *parent)
{
	return undoStack.createRedoAction(parent, QCoreApplication::translate("Command", "&Redo"));
}

// return a string that can be used for the commit message and should list the changes that
// were made to the dive list
QString changesMade()
{
	QString changeTexts;
	for (int i = 0; i < undoStack.index(); i++)
		changeTexts += undoStack.text(i) + "\n";
	return changeTexts;
}

bool execute(Base *cmd)
{
	if (cmd->workToBeDone()) {
		undoStack.push(cmd);
		emit diveListNotifier.commandExecuted();
		return true;
	} else {
		delete cmd;
		return false;
	}
}

} // namespace Command


