
// SPDX-License-Identifier: GPL-2.0
#include <QApplication>
#include <QNetworkProxy>
#include <QLibraryInfo>
#include <QTextCodec>
#include <QTranslator>
#include "qthelper.h"
#include "errorhelper.h"
#include "core/settings/qPref.h"

char *settings_suffix = NULL;
static QTranslator qtTranslator, ssrfTranslator, parentLanguageTranslator;

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
			report_info("using custom config for Subsurface-Mobile-%s", settings_suffix);
#else
			report_info("using custom config for Subsurface-%s", settings_suffix);
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0) && QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	QCoreApplication::setAttribute(Qt::AA_DisableWindowContextHelpButton);
#endif
	qPref::load();

	// find plugins installed in the application directory (without this SVGs don't work on Windows)
	QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());
	QLocale loc;

	// assign en_GB for use in South African locale
	if (loc.country() == QLocale::SouthAfrica) {
		loc.setDefault(QLocale("en_GB"));
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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	translationLocation = QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#else
	translationLocation = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
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
				report_info("can't find Qt base localization for locale %s searching in %s", qPrintable(uiLang), qPrintable(translationLocation));
			}
		}
	}
	// when creating our language names, we didn't define generic languages with additional
	// country specific extensions, instead we included the "primary" country in the
	// translation designations. This breaks the way Qt finds fall-back translations.
	// Changing this now seems rather tedious, so instead we manually create a few
	// obvious fallbacks
	QPair<QLocale::Language, QLocale::Country> parents[] = {
		{ QLocale::German, QLocale::Germany },
		{ QLocale::Portuguese, QLocale::Portugal },
		{ QLocale::French, QLocale::France},
		{ QLocale::Spanish, QLocale::Spain}
	};
	for (auto parent: parents) {
		if (loc.language() == parent.first && loc.country() != parent.second) {
			// first load de_DE so it's used as fall-back
			QLocale parentLoc = QLocale(parent.first, parent.second);
			if (parentLanguageTranslator.load(parentLoc, "subsurface", "_") ||
			    parentLanguageTranslator.load(parentLoc, "subsurface", "_", translationLocation) ||
			    parentLanguageTranslator.load(parentLoc, "subsurface", "_", getSubsurfaceDataPath("translations")) ||
			    parentLanguageTranslator.load(parentLoc, "subsurface", "_", getSubsurfaceDataPath("../translations"))) {
				if (verbose)
					report_info("loading %s translations", qPrintable(parentLoc.name()));
				application->installTranslator(&parentLanguageTranslator);
			} else {
				report_info("can't find Subsurface localization for locale %s", qPrintable(parentLoc.name()));
			}
		}

	}
	if (ssrfTranslator.load(loc, "subsurface", "_") ||
	    ssrfTranslator.load(loc, "subsurface", "_", translationLocation) ||
	    ssrfTranslator.load(loc, "subsurface", "_", getSubsurfaceDataPath("translations")) ||
	    ssrfTranslator.load(loc, "subsurface", "_", getSubsurfaceDataPath("../translations"))) {
		if (verbose)
			report_info("loading %s translations", qPrintable(uiLang));
		application->installTranslator(&ssrfTranslator);
	} else {
		report_info("can't find Subsurface localization for locale %s", qPrintable(uiLang));
	}
}
