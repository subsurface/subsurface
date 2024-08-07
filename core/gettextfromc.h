// SPDX-License-Identifier: GPL-2.0
#ifndef GETTEXTFROMC_H
#define GETTEXTFROMC_H

#include <QCoreApplication>

const char *trGettext(const char *text);

class gettextFromC {
	Q_DECLARE_TR_FUNCTIONS(gettextFromC)
};

#endif // GETTEXTFROMC_H
