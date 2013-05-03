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

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QStringList>
#include <QTextCodec>
#include <QTranslator>

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

void init_qt_ui(int *argcp, char ***argvp)
{
	application->installTranslator(new Translator(application));
	MainWindow *window = new MainWindow();
	window->show();
}

void init_ui(int *argcp, char ***argvp)
{
	application = new QApplication(*argcp, *argvp);

#if QT_VERSION < 0x050000
	// ask QString in Qt 4 to interpret all char* as UTF-8,
	// like Qt 5 does.
	// 106 is "UTF-8", this is faster than lookup by name
	// [http://www.iana.org/assignments/character-sets/character-sets.xml]
	QTextCodec::setCodecForCStrings(QTextCodec::codecForMib(106));
#endif

	subsurface_open_conf();

	load_preferences();

	default_dive_computer_vendor = subsurface_get_conf("dive_computer_vendor");
	default_dive_computer_product = subsurface_get_conf("dive_computer_product");
	default_dive_computer_device = subsurface_get_conf("dive_computer_device");

	return;
}

void run_ui(void)
{
	application->exec();
}

void exit_ui(void)
{
	delete application;
	subsurface_close_conf();
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


#include "qt-gui.moc"
