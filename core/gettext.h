// SPDX-License-Identifier: GPL-2.0
#ifndef MYGETTEXT_H
#define MYGETTEXT_H

const char *trGettext(const char *);
static inline const char *translate(const char *, const char *arg)
{
	return trGettext(arg);
}

#endif // MYGETTEXT_H
