/*
 * common.h
 *
 * shared by all Qt/C++ code
 *
 */

#ifndef COMMON_H
#define COMMON_H

#include <glib/gi18n.h>
#include <QString>

/* use this instead of tr() for translated string literals */
inline QString Qtr_(const char *str)
{
	return QString::fromUtf8(gettext(str));
}

#endif
