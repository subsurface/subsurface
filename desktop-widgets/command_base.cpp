// SPDX-License-Identifier: GPL-2.0

#include "command_base.h"
#include "core/qthelper.h" // for updateWindowTitle()

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

void execute(Base *cmd)
{
	if (cmd->workToBeDone())
		undoStack.push(cmd);
	else
		delete cmd;
}

} // namespace Command


