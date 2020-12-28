// SPDX-License-Identifier: GPL-2.0
#include <QApplication>
#include <Qt>
#include <QNetworkProxy>
#include <QLibraryInfo>
#include <QTextCodec>
#include <QTranslator>
#include "qthelper.h"
#include "errorhelper.h"
#include "core/settings/qPref.h"

char *settings_suffix = NULL;
static QTranslator qtTranslator, ssrfTranslator;

void init_qt_late()
{
	QApplication *application = qApp;
	// tell Qt to use system proxies
	// note: on Linux, "system" == "environment variables"
	QNetworkProxyFactory::setUseSystemConfiguration(true);

	// Set the locale codec to UTF-8.
	// This makes QFile::encodeName() work on Windows and qPrintable() is equivalent to qUtf8Printable().
	QTextCodec::setCodecForLocale(QTextCodec::codecForMib(106));

	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
#if defined(Q_OS_LINUX)
	QGuiApplication::setDesktopFileName("subsurface");
#endif
	// enable user specific settings (based on command line argument)
	if (settings_suffix) {
		if (verbose)
#if defined(SUBSURFACE_MOBILE) && ((defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || (defined(Q_OS_DARWIN) && !defined(Q_OS_IOS)))
			qDebug() << "using custom config for" << QString("Subsurface-Mobile-%1").arg(settings_suffix);
#else
			qDebug() << "using custom config for" << QString("Subsurface-%1").arg(settings_suffix);
#endif
#if defined(SUBSURFACE_MOBILE) && ((defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || (defined(Q_OS_DARWIN) && !defined(Q_OS_IOS)))
		QCoreApplication::setApplicationName(QString("Subsurface-Mobile-%1").arg(settings_suffix));
#else
		QCoreApplication::setApplicationName(QString("Subsurface-%1").arg(settings_suffix));
#endif
	} else {
#if defined(SUBSURFACE_MOBILE) && ((defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || (defined(Q_OS_DARWIN) && !defined(Q_OS_IOS)))
		QCoreApplication::setApplicationName("Subsurface-Mobile");
#else
		QCoreApplication::setApplicationName("Subsurface");
#endif
	}
	// Disables the WindowContextHelpButtonHint by default on Qt::Sheet and Qt::Dialog widgets.
	// This hides the ? button on Windows, which only makes sense if you use QWhatsThis functionality.
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
	QCoreApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif
	qPref::load();

	// find plugins installed in the application directory (without this SVGs don't work on Windows)
	QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());
	QLocale loc;

	// assign en_GB for use in South African locale
	// and capture other French and Spanish speaking countries with the corresponding canonical locales
	if (loc.country() == QLocale::SouthAfrica) {
		loc.setDefault(QLocale("en_GB"));
		loc = QLocale();
	} else if (loc.language() == QLocale::French) {
		loc.setDefault(QLocale("fr_FR"));
		loc = QLocale();
	} else if (loc.language() == QLocale::Spanish) {
		loc.setDefault(QLocale("es_ES"));
		loc = QLocale();
	} else if (loc.language() == QLocale::German && loc.country() != QLocale::Switzerland) {
		loc.setDefault(QLocale("de_DE"));
		loc = QLocale();
	}
	initUiLanguage();
	QString uiLang = getUiLanguage();
	loc = getLocale();
	QLocale::setDefault(loc);

	QString translationLocation;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
	translationLocation = QLatin1String(":/");
#else
	translationLocation = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
	if (uiLang != "en_US" && uiLang != "en-US") {
		if (qtTranslator.load(loc, "qtbase", "_", translationLocation) ||
		    qtTranslator.load(loc, "qtbase", "_", getSubsurfaceDataPath("translations")) ||
		    qtTranslator.load(loc, "qtbase", "_", getSubsurfaceDataPath("../translations"))) {
			application->installTranslator(&qtTranslator);
		} else {
			// it's possible that this is one of the couple of languages that still have qt_XX translations
			// and no qtbase_XX translations - as of this writing this is true for Swedish and Portuguese
			if (qtTranslator.load(loc, "qt", "_", translationLocation) ||
			    qtTranslator.load(loc, "qt", "_", getSubsurfaceDataPath("translations")) ||
			    qtTranslator.load(loc, "qt", "_", getSubsurfaceDataPath("../translations"))) {
				application->installTranslator(&qtTranslator);
			} else {
				qDebug() << "can't find Qt base localization for locale" << uiLang << "searching in" << translationLocation;
			}
		}
	}
	if (ssrfTranslator.load(loc, "subsurface", "_") ||
	    ssrfTranslator.load(loc, "subsurface", "_", translationLocation) ||
	    ssrfTranslator.load(loc, "subsurface", "_", getSubsurfaceDataPath("translations")) ||
	    ssrfTranslator.load(loc, "subsurface", "_", getSubsurfaceDataPath("../translations"))) {
		application->installTranslator(&ssrfTranslator);
	} else {
		qDebug() << "can't find Subsurface localization for locale" << uiLang;
	}
}
