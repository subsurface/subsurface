// SPDX-License-Identifier: GPL-2.0

#include "command_base.h"
#include "core/qthelper.h" // for updateWindowTitle()
#include "core/subsurface-qt/DiveListNotifier.h"

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

QAction *undoAction(QObject *parent)
{
	return undoStack.createUndoAction(parent, QCoreApplication::translate("Command", "&Undo"));
}

QAction *redoAction(QObject *parent)
{
	return undoStack.createRedoAction(parent, QCoreApplication::translate("Command", "&Redo"));
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


