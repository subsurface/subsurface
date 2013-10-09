#include <QCoreApplication>
#include <QString>
#include <gettextfromc.h>

const char *gettextFromC::gettext(const char *text)
{
	QByteArray &result = translationCache[text];
	if (result.isEmpty())
		result = tr(text).toUtf8();
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

extern "C" const char *gettext(const char *text)
{
	return gettextFromC::instance()->gettext(text);
}
