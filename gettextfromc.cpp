#include <QCoreApplication>
#include <QString>
#include <gettextfromc.h>

char *gettextFromC::gettext(const char *text)
{
	return tr(text).toLocal8Bit().data();
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
