// SPDX-License-Identifier: GPL-2.0
#include "gettextfromc.h"
#include <QHash>

static QHash<QByteArray, QByteArray> translationCache;

extern "C" const char *trGettext(const char *text)
{
	QByteArray &result = translationCache[QByteArray(text)];
	if (result.isEmpty())
		result = gettextFromC::tr(text).toUtf8();
	return result.constData();
}
