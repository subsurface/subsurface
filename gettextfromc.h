#ifndef GETTEXTFROMC_H
#define GETTEXTFROMC_H

#include <QHash>
#include <QCoreApplication>

extern "C" const char *trGettext(const char *text);

class gettextFromC {
	Q_DECLARE_TR_FUNCTIONS(gettextFromC)
public:
	static gettextFromC *instance();
	const char *trGettext(const char *text);
	void reset(void);
	QHash<QByteArray, QByteArray> translationCache;
};

#endif // GETTEXTFROMC_H
