// SPDX-License-Identifier: GPL-2.0

#include "command.h"
#include "command_base.h"
#include "core/divelog.h"
#include "core/globals.h"
#include "core/qthelper.h" // for updateWindowTitle()
#include "core/subsurface-qt/divelistnotifier.h"
#include <QVector>

namespace Command {

static QUndoStack *undoStack;

// forward declaration
QString changesMade();

// General commands
void init()
{
	undoStack = make_global<QUndoStack>();
	QObject::connect(undoStack, &QUndoStack::cleanChanged, &updateWindowTitle);
	QObject::connect(&diveListNotifier, &DiveListNotifier::dataReset, &Command::clear);
	changesCallback = &changesMade;
}

void clear()
{
	undoStack->clear();
}

void setClean()
{
	undoStack->setClean();
}

bool isClean()
{
	return undoStack->isClean();
}

// this can be used to get access to the signals emitted by the QUndoStack
QUndoStack *getUndoStack()
{
	return undoStack;
}

QAction *undoAction(QObject *parent)
{
	return undoStack->createUndoAction(parent, QCoreApplication::translate("Command", "&Undo"));
}

QAction *redoAction(QObject *parent)
{
	return undoStack->createRedoAction(parent, QCoreApplication::translate("Command", "&Redo"));
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
	if (dives.size() == divelog.dives.size())
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
// keep in mind that the changes could have been a number of undo commands, so we might have
// to go backwards from the last one written; this isn't perfect as a user could undo a command
// and then do something else instead of redoing that undo - the undo information is then lost
// for the changelog -- but of course the git history will show what happened
QString changesMade()
{
	static int nextToWrite = 0;
	int curIdx = undoStack->index();
	QString changeTexts;

	if (curIdx > nextToWrite) {
		for (int i = nextToWrite; i < curIdx; i++)
			changeTexts += undoStack->text(i) + "\n";
	} else if (curIdx < nextToWrite) { // we walked back undoing things
		for (int i = nextToWrite - 1; i >= curIdx; i--)
			changeTexts += "(undo) " + undoStack->text(i) + "\n";
	} else if (curIdx > 0) {
		// so this means we undid something (or more than one thing) and then did something else
		// so we lost the information of what was undone - and how many things were changed;
		// just show the last change
		changeTexts += undoStack->text(curIdx - 1) + "\n";
	}
	nextToWrite = curIdx;
	return changeTexts;
}

static bool executingCommand = false;
bool execute(Base *cmd)
{
	if (cmd->workToBeDone()) {
		executingCommand = true;
		undoStack->push(cmd);
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
