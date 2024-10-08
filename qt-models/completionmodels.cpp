// SPDX-License-Identifier: GPL-2.0
#include "qt-models/completionmodels.h"
#include "core/dive.h"
#include "core/divelist.h"
#include "core/divelog.h"
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

static QStringList getCSVList(const std::string dive::*item)
{
	QSet<QString> set;
	for (auto &dive: divelog.dives) {
		QString str = QString::fromStdString(dive.get()->*item);
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

QStringList DiveGuideCompletionModel::getStrings()
{
	return getCSVList(&dive::diveguide);
}

bool DiveGuideCompletionModel::relevantDiveField(const DiveField &f)
{
	return f.diveguide;
}

QStringList SuitCompletionModel::getStrings()
{
	QStringList list;
	for (auto &dive: divelog.dives) {
		QString suit = QString::fromStdString(dive->suit);
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
	QStringList list;
	for (const std::unique_ptr<divetag> &tag: g_tag_list)
		list.append(QString::fromStdString(tag->name));
	std::sort(list.begin(), list.end());
	return list;
}

bool TagCompletionModel::relevantDiveField(const DiveField &f)
{
	return f.tags;
}
