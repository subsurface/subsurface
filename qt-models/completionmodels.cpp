// SPDX-License-Identifier: GPL-2.0
#include "qt-models/completionmodels.h"
#include "core/dive.h"
#include "core/tag.h"
#include <QSet>
#include <QString>

#define CREATE_UPDATE_METHOD(Class, diveStructMember)          \
	void Class::updateModel()                              \
	{                                                      \
		QStringList list;                              \
		struct dive *dive;                             \
		int i = 0;                                     \
		for_each_dive (i, dive)                        \
		{                                              \
			QString buddy(dive->diveStructMember); \
			if (!list.contains(buddy)) {           \
				list.append(buddy);            \
			}                                      \
		}                                              \
		std::sort(list.begin(), list.end());           \
		setStringList(list);                           \
	}

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define SKIP_EMPTY Qt::SkipEmptyParts
#else
#define SKIP_EMPTY QString::SkipEmptyParts
#endif

#define CREATE_CSV_UPDATE_METHOD(Class, diveStructMember)                                        \
	void Class::updateModel()                                                                \
	{                                                                                        \
		QSet<QString> set;                                                               \
		struct dive *dive;                                                               \
		int i = 0;                                                                       \
		for_each_dive (i, dive)                                                          \
		{                                                                                \
			QString buddy(dive->diveStructMember);                                   \
			foreach (const QString &value, buddy.split(",", SKIP_EMPTY))             \
			{                                                                        \
				set.insert(value.trimmed());                                     \
			}                                                                        \
		}                                                                                \
		QStringList setList = set.values();                                              \
		std::sort(setList.begin(), setList.end());                                       \
		setStringList(setList);                                                     \
	}

CREATE_CSV_UPDATE_METHOD(BuddyCompletionModel, buddy);
CREATE_CSV_UPDATE_METHOD(DiveMasterCompletionModel, divemaster);
CREATE_UPDATE_METHOD(SuitCompletionModel, suit);

void TagCompletionModel::updateModel()
{
	if (g_tag_list == NULL)
		return;
	QStringList list;
	struct tag_entry *current_tag_entry = g_tag_list;
	while (current_tag_entry != NULL) {
		list.append(QString(current_tag_entry->tag->name));
		current_tag_entry = current_tag_entry->next;
	}
	setStringList(list);
}
