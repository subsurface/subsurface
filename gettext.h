#ifndef MYGETTEXT_H
#define MYGETTEXT_H

/* this is for the Qt based translations */
extern const char *gettext(const char *);
#define tr(arg) gettext(arg)
#define QT_TR_NOOP(arg) arg

#endif // MYGETTEXT_H
