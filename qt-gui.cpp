/* qt-gui.cpp */
/* Qt UI implementation */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <ctype.h>

#include "dive.h"
#include "divelist.h"
#include "display.h"
#include "uemis.h"
#include "device.h"
#include "webservice.h"
#include "libdivecomputer.h"
#include "qt-ui/mainwindow.h"
#include "helpers.h"
#include "qthelper.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QStringList>
#include <QTextCodec>
#include <QTranslator>
#include <QSettings>
#include <QDesktopWidget>
#include <QStyle>
#include <QMap>
#include <QMultiMap>
#include <QNetworkProxy>
#include <QDateTime>
#include <QRegExp>
#include <QResource>
#include <QLibraryInfo>

#include <gettextfromc.h>

// this will create a warning when executing lupdate
#define translate(_context, arg) gettextFromC::instance()->tr(arg)

static QApplication *application = NULL;
static MainWindow *window = NULL;

int error_count;
const char *existing_filename;
static QString shortDateFormat;
static QString dateFormat;
static QString timeFormat;
static QLocale loc;

#if defined(Q_OS_WIN) && QT_VERSION < 0x050000
static QByteArray encodeUtf8(const QString &fname)
{
	return fname.toUtf8();
}

static QString decodeUtf8(const QByteArray &fname)
{
	return QString::fromUtf8(fname);
}
#endif

void init_qt(int *argcp, char ***argvp)
{
	application = new QApplication(*argcp, *argvp);
}

QString uiLanguage(QLocale *callerLoc)
{
	QSettings s;
	s.beginGroup("Language");

	if (!s.value("UseSystemLanguage", true).toBool()) {
		loc = QLocale(s.value("UiLanguage", QLocale().uiLanguages().first()).toString());
	} else {
		loc = QLocale(QLocale().uiLanguages().first());
	}

	QString uiLang = loc.uiLanguages().first();
	s.endGroup();

	// there's a stupid Qt bug on MacOS where uiLanguages doesn't give us the country info
	if (!uiLang.contains('-') && uiLang != loc.bcp47Name()) {
		QLocale loc2(loc.bcp47Name());
		loc = loc2;
		uiLang = loc2.uiLanguages().first();
	}
	if (callerLoc)
		*callerLoc = loc;

	// the short format is fine
	// the long format uses long weekday and month names, so replace those with the short ones
	// for time we don't want the time zone designator and don't want leading zeroes on the hours
	shortDateFormat = loc.dateFormat(QLocale::ShortFormat);
	dateFormat = loc.dateFormat(QLocale::LongFormat);
	dateFormat.replace("dddd,", "ddd").replace("dddd", "ddd").replace("MMMM", "MMM");
	// special hack for Swedish as our switching from long weekday names to short weekday names
	// messes things up there
	dateFormat.replace("'en' 'den' d:'e'", " d");
	timeFormat = loc.timeFormat();
	timeFormat.replace("(t)", "").replace(" t", "").replace("t", "").replace("hh", "h").replace("HH", "H").replace("'kl'.", "");
	timeFormat.replace(".ss", "").replace(":ss", "").replace("ss", "");
	return uiLang;
}

QLocale getLocale()
{
	return loc;
}

QString getDateFormat()
{
	return dateFormat;
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
	free((void *)default_dive_computer_device);
}

void set_filename(const char *filename, bool force)
{
	if (!force && existing_filename)
		return;
	free((void *)existing_filename);
	if (filename)
		existing_filename = strdup(filename);
	else
		existing_filename = NULL;
}

const QString get_dc_nickname(const char *model, uint32_t deviceid)
{
	const DiveComputerNode *existNode = dcList.getExact(model, deviceid);

	if (existNode && !existNode->nickName.isEmpty())
		return existNode->nickName;
	else
		return model;
}

QString get_depth_string(int mm, bool showunit, bool showdecimal)
{
	if (prefs.units.length == units::METERS) {
		double meters = mm / 1000.0;
		return QString("%1%2").arg(meters, 0, 'f', (showdecimal && meters < 20.0) ? 1 : 0).arg(showunit ? translate("gettextFromC", "m") : "");
	} else {
		double feet = mm_to_feet(mm);
		return QString("%1%2").arg(feet, 0, 'f', 0).arg(showunit ? translate("gettextFromC", "ft") : "");
	}
}

QString get_depth_string(depth_t depth, bool showunit, bool showdecimal)
{
	return get_depth_string(depth.mm, showunit, showdecimal);
}

QString get_depth_unit()
{
	if (prefs.units.length == units::METERS)
		return QString("%1").arg(translate("gettextFromC", "m"));
	else
		return QString("%1").arg(translate("gettextFromC", "ft"));
}

QString get_weight_string(weight_t weight, bool showunit)
{
	QString str = weight_string(weight.grams);
	if (get_units()->weight == units::KG) {
		str = QString("%1%2").arg(str).arg(showunit ? translate("gettextFromC", "kg") : "");
	} else {
		str = QString("%1%2").arg(str).arg(showunit ? translate("gettextFromC", "lbs") : "");
	}
	return (str);
}

QString get_weight_unit()
{
	if (prefs.units.weight == units::KG)
		return QString("%1").arg(translate("gettextFromC", "kg"));
	else
		return QString("%1").arg(translate("gettextFromC", "lbs"));
}

/* these methods retrieve used gas per cylinder */
static unsigned start_pressure(cylinder_t *cyl)
{
	return cyl->start.mbar ?: cyl->sample_start.mbar;
}

static unsigned end_pressure(cylinder_t *cyl)
{
	return cyl->end.mbar ?: cyl->sample_end.mbar;
}

QString get_cylinder_used_gas_string(cylinder_t *cyl, bool showunit)
{
	int decimals;
	const char *unit;
	double gas_usage;
	/* Get the cylinder gas use in mbar */
	gas_usage = start_pressure(cyl) - end_pressure(cyl);
	/* Can we turn it into a volume? */
	if (cyl->type.size.mliter) {
		gas_usage = bar_to_atm(gas_usage / 1000);
		gas_usage *= cyl->type.size.mliter;
		gas_usage = get_volume_units(gas_usage, &decimals, &unit);
	} else {
		gas_usage = get_pressure_units(gas_usage, &unit);
		decimals = 0;
	}
	// translate("gettextFromC","%.*f %s"
	return QString("%1 %2").arg(gas_usage, 0, 'f', decimals).arg(showunit ? unit : "");
}

QString get_temperature_string(temperature_t temp, bool showunit)
{
	if (temp.mkelvin == 0) {
		return ""; //temperature not defined
	} else if (prefs.units.temperature == units::CELSIUS) {
		double celsius = mkelvin_to_C(temp.mkelvin);
		return QString("%1%2%3").arg(celsius, 0, 'f', 1).arg(showunit ? (UTF8_DEGREE) : "").arg(showunit ? translate("gettextFromC", "C") : "");
	} else {
		double fahrenheit = mkelvin_to_F(temp.mkelvin);
		return QString("%1%2%3").arg(fahrenheit, 0, 'f', 1).arg(showunit ? (UTF8_DEGREE) : "").arg(showunit ? translate("gettextFromC", "F") : "");
	}
}

QString get_temp_unit()
{
	if (prefs.units.temperature == units::CELSIUS)
		return QString(UTF8_DEGREE "C");
	else
		return QString(UTF8_DEGREE "F");
}

QString get_volume_string(volume_t volume, bool showunit, int mbar)
{
	const char *unit;
	int decimals;
	double value = get_volume_units(volume.mliter, &decimals, &unit);
	if (mbar) {
		// we are showing a tank size
		// fix the weird imperial way of denominating size and provide
		// reasonable number of decimals
		if (prefs.units.volume == units::CUFT)
			value *= bar_to_atm(mbar / 1000.0);
		decimals = (value > 20.0) ? 0 : (value > 2.0) ? 1 : 2;
	}
	return QString("%1%2").arg(value, 0, 'f', decimals).arg(showunit ? unit : "");
}

QString get_volume_unit()
{
	const char *unit;
	(void) get_volume_units(0, NULL, &unit);
	return QString(unit);
}

QString get_pressure_string(pressure_t pressure, bool showunit)
{
	if (prefs.units.pressure == units::BAR) {
		double bar = pressure.mbar / 1000.0;
		return QString("%1%2").arg(bar, 0, 'f', 1).arg(showunit ? translate("gettextFromC", "bar") : "");
	} else {
		double psi = mbar_to_PSI(pressure.mbar);
		return QString("%1%2").arg(psi, 0, 'f', 0).arg(showunit ? translate("gettextFromC", "psi") : "");
	}
}

double get_screen_dpi()
{
	QDesktopWidget *mydesk = application->desktop();
	return mydesk->physicalDpiX();
}

QString getSubsurfaceDataPath(QString folderToFind)
{
	QString execdir;
	QDir folder;

	// first check if we are running in the build dir, so the path that we
	// are looking for is just a  subdirectory of the execution path;
	// this also works on Windows as there we install the dirs
	// under the application path
	execdir = QCoreApplication::applicationDirPath();
	folder = QDir(execdir.append(QDir::separator()).append(folderToFind));
	if (folder.exists())
		return folder.absolutePath();

	// next check for the Linux typical $(prefix)/share/subsurface
	execdir = QCoreApplication::applicationDirPath();
	if (execdir.contains("bin")) {
		folder = QDir(execdir.replace("bin", "share/subsurface/").append(folderToFind));
		if (folder.exists())
			return folder.absolutePath();
	}
	// then look for the usual locations on a Mac
	execdir = QCoreApplication::applicationDirPath();
	folder = QDir(execdir.append("/../Resources/share/").append(folderToFind));
	if (folder.exists())
		return folder.absolutePath();
	execdir = QCoreApplication::applicationDirPath();
	folder = QDir(execdir.append("/../Resources/").append(folderToFind));
	if (folder.exists())
		return folder.absolutePath();
	return QString("");
}

int gettimezoneoffset(timestamp_t when)
{
	QDateTime dt1, dt2;
	if (when == 0)
		dt1 = QDateTime::currentDateTime();
	else
		dt1 = QDateTime::fromMSecsSinceEpoch(when * 1000);
	dt2 = dt1.toUTC();
	dt1.setTimeSpec(Qt::UTC);
	return dt2.secsTo(dt1);
}

int parseTemperatureToMkelvin(const QString &text)
{
	int mkelvin;
	QString numOnly = text;
	numOnly.replace(",", ".").remove(QRegExp("[^-0-9.]"));
	if (numOnly.isEmpty())
		return 0;
	double number = numOnly.toDouble();
	switch (prefs.units.temperature) {
	case units::CELSIUS:
		mkelvin = C_to_mkelvin(number);
		break;
	case units::FAHRENHEIT:
		mkelvin = F_to_mkelvin(number);
		break;
	default:
		mkelvin = 0;
	}
	return mkelvin;
}

QString get_dive_date_string(timestamp_t when)
{
	QDateTime ts;
	ts.setMSecsSinceEpoch(when * 1000L);
	return loc.toString(ts.toUTC(), dateFormat + " " + timeFormat);
}

QString get_short_dive_date_string(timestamp_t when)
{
	QDateTime ts;
	ts.setMSecsSinceEpoch(when * 1000L);
	return loc.toString(ts.toUTC(), shortDateFormat + " " + timeFormat);
}

const char *get_dive_date_c_string(timestamp_t when)
{
	QString text = get_dive_date_string(when);
	return strdup(text.toUtf8().data());
}

QString get_trip_date_string(timestamp_t when, int nr)
{
	struct tm tm;
	utc_mkdate(when, &tm);
	if (nr != 1)
		return translate("gettextFromC", "%1 %2 (%3 dives)")
			.arg(monthname(tm.tm_mon))
			.arg(tm.tm_year + 1900)
			.arg(nr);
	else
		return translate("gettextFromC", "%1 %2 (1 dive)")
			.arg(monthname(tm.tm_mon))
			.arg(tm.tm_year + 1900);
}
