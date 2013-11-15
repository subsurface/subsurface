#include <QCoreApplication>
#include <QString>
#include <gettextfromc.h>

const char *gettextFromC::trGettext(const char *text)
{
	QByteArray &result = translationCache[QByteArray(text)];
	if (result.isEmpty())
		result = trUtf8(text).toUtf8();
	return result.constData();
}

void gettextFromC::reset(void)
{
	translationCache.clear();
}

gettextFromC* gettextFromC::instance()
{
	static gettextFromC *self = new gettextFromC();
	return self;
}

extern "C" const char *trGettext(const char *text)
{
	return gettextFromC::instance()->trGettext(text);
}
