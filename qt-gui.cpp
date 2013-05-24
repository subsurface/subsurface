/* qt-gui.cpp */
/* Qt UI implementation */
#include <libintl.h>
#include <glib/gi18n.h>
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
#include "version.h"
#include "libdivecomputer.h"
#include "qt-ui/mainwindow.h"
#include "helpers.h"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QStringList>
#include <QTextCodec>
#include <QTranslator>
#include <QSettings>
#include <QDesktopWidget>

const char *default_dive_computer_vendor;
const char *default_dive_computer_product;
const char *default_dive_computer_device;

class Translator: public QTranslator
{
	Q_OBJECT

public:
	Translator(QObject *parent = 0);
	~Translator() {}

	virtual QString	translate(const char *context, const char *sourceText,
				  const char *disambiguation = NULL) const;
};

Translator::Translator(QObject *parent):
	QTranslator(parent)
{
}

QString Translator::translate(const char *context, const char *sourceText,
			      const char *disambiguation) const
{
	return gettext(sourceText);
}

static QApplication *application = NULL;

int        error_count;
const char *existing_filename;

void init_qt_ui(int *argcp, char ***argvp, char *errormessage)
{
	application->installTranslator(new Translator(application));
	MainWindow *window = new MainWindow();
	window->showError(errormessage);
	window->show();
}

const char *getSetting(QSettings &s, QString name)
{
	QVariant v;
	v = s.value(name);
	if (v.isValid()) {
		return strdup(v.toString().toUtf8().constData());
	}
	return NULL;
}

void init_ui(int *argcp, char ***argvp)
{
	QVariant v;
	application = new QApplication(*argcp, *argvp);

#if QT_VERSION < 0x050000
	// ask QString in Qt 4 to interpret all char* as UTF-8,
	// like Qt 5 does.
	// 106 is "UTF-8", this is faster than lookup by name
	// [http://www.iana.org/assignments/character-sets/character-sets.xml]
	QTextCodec::setCodecForCStrings(QTextCodec::codecForMib(106));
#endif
	QCoreApplication::setOrganizationName("Subsurface");
	QCoreApplication::setOrganizationDomain("subsurface.hohndel.org");
	QCoreApplication::setApplicationName("Subsurface");

	QSettings s;
	s.beginGroup("GeneralSettings");
	prefs.default_filename = getSetting(s, "default_filename");
	s.endGroup();
	s.beginGroup("DiveComputer");
	default_dive_computer_vendor = getSetting(s, "dive_computer_vendor");
	default_dive_computer_product = getSetting(s,"dive_computer_product");
	default_dive_computer_device = getSetting(s, "dive_computer_device");
	s.endGroup();
	return;
}

void run_ui(void)
{
	application->exec();
}

void exit_ui(void)
{
	delete application;
	if (existing_filename)
		free((void *)existing_filename);
	if (default_dive_computer_device)
		free((void *)default_dive_computer_device);
}

void set_filename(const char *filename, gboolean force)
{
	if (!force && existing_filename)
		return;
	free((void *)existing_filename);
	if (filename)
		existing_filename = strdup(filename);
	else
		existing_filename = NULL;
}

const char *get_dc_nickname(const char *model, uint32_t deviceid)
{
	struct device_info *known = get_device_info(model, deviceid);
	if (known) {
		if (known->nickname && *known->nickname)
			return known->nickname;
		else
			return known->model;
	}
	return NULL;
}

void set_dc_nickname(struct dive *dive)
{
	/* needs Qt implementation */
}

QString get_depth_string(depth_t depth, bool showunit)
{
	if (prefs.units.length == units::METERS) {
		double meters = depth.mm / 1000.0;
		return QString("%1%2").arg(meters, 0, 'f', meters >= 20.0 ? 0 : 1 ).arg(showunit ? _("m") : "");
	} else {
		double feet = mm_to_feet(depth.mm);
		return QString("%1%2").arg(feet, 0, 'f', 1). arg(showunit ? _("ft") : "");
	}
}

QString get_weight_string(weight_t weight, bool showunit)
{
	if (prefs.units.weight == units::KG) {
		double kg = weight.grams / 1000.0;
		return QString("%1%2").arg(kg, 0, 'f', kg >= 20.0 ? 0 : 1 ).arg(showunit ? _("kg") : "");
	} else {
		double lbs = grams_to_lbs(weight.grams);
		return QString("%1%2").arg(lbs, 0, 'f', lbs >= 40.0 ? 0 : 1 ).arg(showunit ? _("lbs") : "");
	}
}

QString get_temperature_string(temperature_t temp, bool showunit)
{
	if (prefs.units.temperature == units::CELSIUS) {
		double celsius = mkelvin_to_C(temp.mkelvin);
		return QString("%1%2%3").arg(celsius, 0, 'f', 1).arg(showunit ? (UTF8_DEGREE): "")
								.arg(showunit ? _("C") : "");
	} else {
		double fahrenheit = mkelvin_to_F(temp.mkelvin);
		return QString("%1%2%3").arg(fahrenheit, 0, 'f', 1).arg(showunit ? (UTF8_DEGREE): "")
								.arg(showunit ? _("F") : "");
	}
}

QString get_volume_string(volume_t volume, bool showunit)
{
	if (prefs.units.volume == units::LITER) {
		double liter = volume.mliter / 1000.0;
		return QString("%1%2").arg(liter, 0, 'f', liter >= 40.0 ? 0 : 1 ).arg(showunit ? _("l") : "");
	} else {
		double cuft = ml_to_cuft(volume.mliter);
		return QString("%1%2").arg(cuft, 0, 'f', cuft >= 20.0 ? 0 : (cuft >= 2.0 ? 1 : 2)).arg(showunit ? _("cuft") : "");
	}
}

QString get_pressure_string(pressure_t pressure, bool showunit)
{
	if (prefs.units.pressure == units::BAR) {
		double bar = pressure.mbar / 1000.0;
		return QString("%1%2").arg(bar, 0, 'f', 1).arg(showunit ? _("bar") : "");
	} else {
		double psi = mbar_to_PSI(pressure.mbar);
		return QString("%1%2").arg(psi, 0, 'f', 0).arg(showunit ? _("psi") : "");
	}
}

double get_screen_dpi()
{
	QDesktopWidget *mydesk = application->desktop();
	return mydesk->physicalDpiX();
}

int is_default_dive_computer(const char *vendor, const char *product)
{
	return default_dive_computer_vendor && !strcmp(vendor, default_dive_computer_vendor) &&
		default_dive_computer_product && !strcmp(product, default_dive_computer_product);
}

int is_default_dive_computer_device(const char *name)
{
	return default_dive_computer_device && !strcmp(name, default_dive_computer_device);
}

void set_default_dive_computer(const char *vendor, const char *product)
{
	QSettings s;

	if (!vendor || !*vendor)
		return;
	if (!product || !*product)
		return;
	if (is_default_dive_computer(vendor, product))
		return;
	if (default_dive_computer_vendor)
		free((void *)default_dive_computer_vendor);
	if (default_dive_computer_product)
		free((void *)default_dive_computer_product);
	default_dive_computer_vendor = strdup(vendor);
	default_dive_computer_product = strdup(product);
	s.beginGroup("DiveComputer");
	s.setValue("dive_computer_vendor", vendor);
	s.setValue("dive_computer_product", product);
	s.endGroup();
}

void set_default_dive_computer_device(const char *name)
{
	QSettings s;

	if (!name || !*name)
		return;
	if (is_default_dive_computer_device(name))
		return;
	if (default_dive_computer_device)
		free((void *)default_dive_computer_device);
	default_dive_computer_device = strdup(name);
	s.beginGroup("DiveComputer");
	s.setValue("dive_computer_device", name);
	s.endGroup();
}

#include "qt-gui.moc"
