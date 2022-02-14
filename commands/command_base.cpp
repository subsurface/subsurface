// SPDX-License-Identifier: GPL-2.0

#include "command_base.h"
#include "core/qthelper.h" // for updateWindowTitle()
#include "core/subsurface-qt/divelistnotifier.h"
#include <QVector>

namespace Command {

static QUndoStack undoStack;

// forward declaration
QString changesMade();

// General commands
void init()
{
	QObject::connect(&undoStack, &QUndoStack::cleanChanged, &updateWindowTitle);
	changesCallback = &changesMade;
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

QString diveNumberOrDate(struct dive *d)
{
	if (d->number != 0)
		return QStringLiteral("#%1").arg(d->number);
	else
		return QStringLiteral("@%1").arg(get_short_dive_date_string(d->when));
}

QString getListOfDives(const std::vector<struct dive*> &dives)
{
	QString listOfDives;
	if ((int)dives.size() == dive_table.nr)
		return Base::tr("all dives");
	int i = 0;
	for (dive *d: dives) {
		// we show a maximum of five dive numbers, or 4 plus ellipsis
		if (++i == 4 && dives.size() >= 5)
			return listOfDives + "...";
		listOfDives += diveNumberOrDate(d) + ", ";
	}
	if (!listOfDives.isEmpty())
		listOfDives.truncate(listOfDives.length() - 2);
	return listOfDives;
}

QString getListOfDives(QVector<struct dive *> dives)
{
	return getListOfDives(std::vector<struct dive *>(dives.begin(), dives.end()));
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

static bool executingCommand = false;
bool execute(Base *cmd)
{
	if (cmd->workToBeDone()) {
		executingCommand = true;
		undoStack.push(cmd);
		executingCommand = false;
		emit diveListNotifier.commandExecuted();
		return true;
	} else {
		delete cmd;
		return false;
	}
}

bool placingCommand()
{
	return executingCommand;
}

} // namespace Command
