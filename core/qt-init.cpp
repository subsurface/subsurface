// SPDX-License-Identifier: GPL-2.0
#include <QApplication>
#include <QNetworkProxy>
#include <QLibraryInfo>
#include <QTextCodec>
#include "helpers.h"
#include "core/subsurface-qt/SettingsObjectWrapper.h"

char *settings_suffix = NULL;
QTranslator *qtTranslator, *ssrfTranslator;

void init_qt_late()
{
	QApplication *application = qApp;
	// tell Qt to use system proxies
	// note: on Linux, "system" == "environment variables"
	QNetworkProxyFactory::setUseSystemConfiguration(true);

	// for Win32 and Qt5 we try to set the locale codec to UTF-8.
	// this makes QFile::encodeName() work.
#ifdef Q_OS_WIN
	QTextCodec::setCodecForLocale(QTextCodec::codecForMib(106));
#endif

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	// enable user specific settings (based on command line argument)
	if (settings_suffix) {
		if (verbose)
			qDebug() << "using custom config for" << QString("Subsurface-%1").arg(settings_suffix);
		QCoreApplication::setApplicationName(QString("Subsurface-%1").arg(settings_suffix));
	} else {
		QCoreApplication::setApplicationName("Subsurface");
	}
	// find plugins installed in the application directory (without this SVGs don't work on Windows)
	SettingsObjectWrapper::instance()->load();

	QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());
	QLocale loc;
	QString uiLang = uiLanguage(&loc);
	QLocale::setDefault(loc);

	qtTranslator = new QTranslator;
	QString translationLocation;
#if defined(Q_OS_ANDROID)
	translationLocation = QLatin1Literal("assets:/translations");
#elif defined(Q_OS_IOS)
	translationLocation = QLatin1Literal(":/translations/");
#else
	translationLocation = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
	if (qtTranslator->load(loc, "qt", "_", translationLocation)) {
		application->installTranslator(qtTranslator);
	} else {
		qDebug() << "can't find Qt localization for locale" << uiLang << "searching in" << translationLocation;
	}
	ssrfTranslator = new QTranslator;
	if (ssrfTranslator->load(loc, "subsurface", "_") ||
	    ssrfTranslator->load(loc, "subsurface", "_", translationLocation) ||
	    ssrfTranslator->load(loc, "subsurface", "_", getSubsurfaceDataPath("translations")) ||
	    ssrfTranslator->load(loc, "subsurface", "_", getSubsurfaceDataPath("../translations"))) {
		application->installTranslator(ssrfTranslator);
	} else {
		qDebug() << "can't find Subsurface localization for locale" << uiLang;
	}
}
