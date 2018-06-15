// SPDX-License-Identifier: GPL-2.0
#include "qPrefGeneral.h"
#include "qPref_private.h"


qPrefGeneral *qPrefGeneral::m_instance = NULL;
qPrefGeneral *qPrefGeneral::instance()
{
	if (!m_instance)
		m_instance = new qPrefGeneral;
	return m_instance;
}


void qPrefGeneral::loadSync(bool doSync)
{
	diskAutoRecalculateThumbnails(doSync);
	diskDefaultCylinder(doSync);
	diskDefaultFilename(doSync);
	diskDefaultFileBehavior(doSync);
	diskDefaultSetPoint(doSync);
	diskO2Consumption(doSync);
	diskPscrRatio(doSync);
	diskUseDefaultFile(doSync);
}


bool qPrefGeneral::autoRecalculateThumbnails() const
{
	return prefs.auto_recalculate_thumbnails;
}
void qPrefGeneral::setAutoRecalculateThumbnails(bool value)
{
	if (value != prefs.auto_recalculate_thumbnails) {
		prefs.auto_recalculate_thumbnails = value;
		diskAutoRecalculateThumbnails(true);
		emit autoRecalculateThumbnailsChanged(value);
	}
}
void qPrefGeneral::diskAutoRecalculateThumbnails(bool doSync)
{
	LOADSYNC_BOOL("/auto_recalculate_thumbnails", auto_recalculate_thumbnails);
}


const QString qPrefGeneral::defaultCylinder() const
{
	return prefs.default_cylinder;
}
void qPrefGeneral::setDefaultCylinder(const QString& value)
{
	if (value != prefs.default_cylinder) {
		COPY_TXT(default_cylinder, value);
		diskDefaultCylinder(true);
		emit defaultCylinderChanged(value);
	}
}
void qPrefGeneral::diskDefaultCylinder(bool doSync)
{
	LOADSYNC_TXT("/default_cylinder", default_cylinder);
}


const QString qPrefGeneral::defaultFilename() const
{
	return prefs.default_filename;
}
void qPrefGeneral::setDefaultFilename(const QString& value)
{
	if (value != prefs.default_filename) {
		COPY_TXT(default_filename, value);
		diskDefaultFilename(true);
		emit defaultFilenameChanged(value);
	}
}
void qPrefGeneral::diskDefaultFilename(bool doSync)
{
	LOADSYNC_TXT("default_filename", default_filename);
}


short qPrefGeneral::defaultFileBehavior() const
{
	return prefs.default_file_behavior;
}
void qPrefGeneral::setDefaultFileBehavior(short value)
{
	if (value != prefs.default_file_behavior ||
		prefs.default_file_behavior != UNDEFINED_DEFAULT_FILE) {

	if (value == UNDEFINED_DEFAULT_FILE) {
		// undefined, so check if there's a filename set and
		// use that, otherwise go with no default file
		prefs.default_file_behavior = QString(prefs.default_filename).isEmpty() ?
										NO_DEFAULT_FILE : LOCAL_DEFAULT_FILE;
	}
	else
		prefs.default_file_behavior = value;
	diskDefaultFileBehavior(true);
	emit defaultFileBehaviorChanged(value);
	}
}
void qPrefGeneral::diskDefaultFileBehavior(bool doSync)
{
	LOADSYNC_INT("/default_file_behavior", default_file_behavior);
	if (!doSync && prefs.default_file_behavior == UNDEFINED_DEFAULT_FILE)
		// undefined, so check if there's a filename set and
		// use that, otherwise go with no default file
		prefs.default_file_behavior = QString(prefs.default_filename).isEmpty() ?
										NO_DEFAULT_FILE : LOCAL_DEFAULT_FILE;
}


int qPrefGeneral::defaultSetPoint() const
{
	return prefs.defaultsetpoint;
}
void qPrefGeneral::setDefaultSetPoint(int value)
{
	if (value != prefs.defaultsetpoint) {
		prefs.defaultsetpoint = value;
		diskDefaultSetPoint(true);
		emit defaultSetPointChanged(value);
	}
}
void qPrefGeneral::diskDefaultSetPoint(bool doSync)
{
	LOADSYNC_INT("/defaultsetpoint", defaultsetpoint);
}


int qPrefGeneral::o2Consumption() const
{
	return prefs.o2consumption;
}
void qPrefGeneral::setO2Consumption(int value)
{
	if (value != prefs.o2consumption) {
		prefs.o2consumption = value;
		diskO2Consumption(true);
		emit o2ConsumptionChanged(value);
	}
}
void qPrefGeneral::diskO2Consumption(bool doSync)
{
	LOADSYNC_INT("/o2consumption", o2consumption);
}


int qPrefGeneral::pscrRatio() const
{
	return prefs.pscr_ratio;
}
void qPrefGeneral::setPscrRatio(int value)
{
	if (value != prefs.pscr_ratio) {
		prefs.pscr_ratio = value;
		diskPscrRatio(true);
		emit pscrRatioChanged(value);
	}
}
void qPrefGeneral::diskPscrRatio(bool doSync)
{
	LOADSYNC_INT("/pscr_ratio", pscr_ratio);
}


bool qPrefGeneral::useDefaultFile() const
{
	return prefs.use_default_file;
}
void qPrefGeneral::setUseDefaultFile(bool value)
{
	if (value != prefs.use_default_file) {
		prefs.use_default_file = value;
		diskUseDefaultFile(true);
		emit useDefaultFileChanged(value);
	}
}
void qPrefGeneral::diskUseDefaultFile(bool doSync)
{
	LOADSYNC_BOOL("/use_default_file", use_default_file);
}
