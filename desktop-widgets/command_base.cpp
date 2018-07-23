// SPDX-License-Identifier: GPL-2.0

#include "command_base.h"

namespace Command {

static QUndoStack undoStack;

// General commands
void clear()
{
	undoStack.clear();
}

QAction *undoAction(QObject *parent)
{
	return undoStack.createUndoAction(parent, QCoreApplication::translate("Command", "&Undo"));
}

QAction *redoAction(QObject *parent)
{
	return undoStack.createRedoAction(parent, QCoreApplication::translate("Command", "&Redo"));
}

void execute(Base *cmd)
{
	if (cmd->workToBeDone())
		undoStack.push(cmd);
	else
		delete cmd;
}

} // namespace Command


