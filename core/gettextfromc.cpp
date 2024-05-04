// SPDX-License-Identifier: GPL-2.0
#include "gettextfromc.h"
#include <QHash>
#include <QMutex>

static QHash<QByteArray, QByteArray> translationCache;
static QMutex lock;

const char *trGettext(const char *text)
{
	QByteArray key(text);
	QMutexLocker l(&lock);
	auto it = translationCache.find(key);
	if (it == translationCache.end())
		it = translationCache.insert(key, gettextFromC::tr(text).toUtf8());
	return it->constData();
}
