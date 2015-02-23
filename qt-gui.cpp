/* qt-gui.cpp */
/* Qt UI implementation */
#include "dive.h"
#include "display.h"
#include "qt-ui/mainwindow.h"
#include "helpers.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QNetworkProxy>
#include <QLibraryInfo>

static QApplication *application = NULL;
static MainWindow *window = NULL;

void init_qt(int *argcp, char ***argvp)
{
	application = new QApplication(*argcp, *argvp);
}

void init_ui(void)
{
	// tell Qt to use system proxies
	// note: on Linux, "system" == "environment variables"
	QNetworkProxyFactory::setUseSystemConfiguration(true);

#if QT_VERSION < 0x050000
	// ask QString in Qt 4 to interpret all char* as UTF-8,
	// like Qt 5 does.
	// 106 is "UTF-8", this is faster than lookup by name
	// [http://www.iana.org/assignments/character-sets/character-sets.xml]
	QTextCodec::setCodecForCStrings(QTextCodec::codecForMib(106));
	// and for reasons I can't understand, I need to do the same again for tr
	// even though that clearly uses C strings as well...
	QTextCodec::setCodecForTr(QTextCodec::codecForMib(106));
#ifdef Q_OS_WIN
	QFile::setDecodingFunction(decodeUtf8);
	QFile::setEncodingFunction(encodeUtf8);
#endif
#else
	// for Win32 and Qt5 we try to set the locale codec to UTF-8.
	// this makes QFile::encodeName() work.
#ifdef Q_OS_WIN
	QTextCodec::setCodecForLocale(QTextCodec::codecForMib(106));
#endif
#endif
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("Subsurface");
	// find plugins installed in the application directory (without this SVGs don't work on Windows)
	QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());
	QLocale loc;
	QString uiLang = uiLanguage(&loc);
	QLocale::setDefault(loc);

	// we don't have translations for English - if we don't check for this
	// Qt will proceed to load the second language in preference order - not what we want
	// on Linux this tends to be en-US, but on the Mac it's just en
	if (!uiLang.startsWith("en") || uiLang.startsWith("en-GB")) {
		qtTranslator = new QTranslator;
		if (qtTranslator->load(loc, "qt", "_", QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
			application->installTranslator(qtTranslator);
		} else {
			qDebug() << "can't find Qt localization for locale" << uiLang << "searching in" << QLibraryInfo::location(QLibraryInfo::TranslationsPath);
		}
		ssrfTranslator = new QTranslator;
		if (ssrfTranslator->load(loc, "subsurface", "_") ||
		    ssrfTranslator->load(loc, "subsurface", "_", getSubsurfaceDataPath("translations")) ||
		    ssrfTranslator->load(loc, "subsurface", "_", getSubsurfaceDataPath("../translations"))) {
			application->installTranslator(ssrfTranslator);
		} else {
			qDebug() << "can't find Subsurface localization for locale" << uiLang;
		}
	}
	window = new MainWindow();
	if (existing_filename && existing_filename[0] != '\0')
		window->setTitle(MWTF_FILENAME);
	else
		window->setTitle(MWTF_DEFAULT);
}

void run_ui(void)
{
	window->show();
	application->exec();
}

void exit_ui(void)
{
	delete window;
	delete application;
	free((void *)existing_filename);
	free((void *)default_dive_computer_vendor);
	free((void *)default_dive_computer_product);
	free((void *)default_dive_computer_device);
}

double get_screen_dpi()
{
	QDesktopWidget *mydesk = application->desktop();
	return mydesk->physicalDpiX();
}


