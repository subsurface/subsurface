#ifndef GETTEXT_H
#define GETTEXT_H


extern "C" const char *gettext(const char *text);

class gettextFromC
{
Q_DECLARE_TR_FUNCTIONS(gettextFromC)
public:
	static gettextFromC *instance();
	char *gettext(const char *text);
};

#endif // GETTEXT_H
