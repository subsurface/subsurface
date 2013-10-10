#ifndef MYGETTEXT_H
#define MYGETTEXT_H

/* this is for the Qt based translations */
extern const char *gettext(const char *);
#define translate(_context, arg) gettext(arg)
#define QT_TRANSLATE_NOOP(_context, arg) arg

#endif // MYGETTEXT_H
