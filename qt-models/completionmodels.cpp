// SPDX-License-Identifier: GPL-2.0
#include "qt-models/completionmodels.h"
#include "core/dive.h"
#include "core/tag.h"
#include <QSet>

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define SKIP_EMPTY Qt::SkipEmptyParts
#else
#define SKIP_EMPTY QString::SkipEmptyParts
#endif

CompletionModelBase::CompletionModelBase()
{
	connect(&diveListNotifier, &DiveListNotifier::dataReset, this, &CompletionModelBase::updateModel);
	connect(&diveListNotifier, &DiveListNotifier::divesImported, this, &CompletionModelBase::updateModel);
	connect(&diveListNotifier, &DiveListNotifier::divesChanged, this, &CompletionModelBase::divesChanged);
}

void CompletionModelBase::updateModel()
{
	setStringList(getStrings());
}

void CompletionModelBase::divesChanged(const QVector<dive *> &, DiveField field)
{
	if (relevantDiveField(field))
		updateModel();
}

static QStringList getCSVList(char *dive::*item)
{
	QSet<QString> set;
	struct dive *dive;
	int i = 0;
	for_each_dive (i, dive) {
		QString str(dive->*item);
		for (const QString &value: str.split(",", SKIP_EMPTY))
			set.insert(value.trimmed());
	}
	QStringList setList = set.values();
	std::sort(setList.begin(), setList.end());
	return setList;
}

QStringList BuddyCompletionModel::getStrings()
{
	return getCSVList(&dive::buddy);
}

bool BuddyCompletionModel::relevantDiveField(const DiveField &f)
{
	return f.buddy;
}

QStringList DiveMasterCompletionModel::getStrings()
{
	return getCSVList(&dive::divemaster);
}

bool DiveMasterCompletionModel::relevantDiveField(const DiveField &f)
{
	return f.divemaster;
}

QStringList SuitCompletionModel::getStrings()
{
	QStringList list;
	struct dive *dive;
	int i = 0;
	for_each_dive (i, dive) {
		QString suit(dive->suit);
		if (!list.contains(suit))
			list.append(suit);
	}
	std::sort(list.begin(), list.end());
	return list;
}

bool SuitCompletionModel::relevantDiveField(const DiveField &f)
{
	return f.suit;
}

QStringList TagCompletionModel::getStrings()
{
	if (g_tag_list == NULL)
		return {};
	QStringList list;
	struct tag_entry *current_tag_entry = g_tag_list;
	while (current_tag_entry != NULL) {
		list.append(QString(current_tag_entry->tag->name));
		current_tag_entry = current_tag_entry->next;
	}
	std::sort(list.begin(), list.end());
	return list;
}

bool TagCompletionModel::relevantDiveField(const DiveField &f)
{
	return f.tags;
}
