// SPDX-License-Identifier: GPL-2.0
#include "diveobjecthelper.h"

#include <QDateTime>
#include <QTextDocument>

#include "core/dive.h"
#include "core/qthelper.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/subsurface-string.h"
#include "core/string-format.h"
#include "qt-models/tankinfomodel.h"

#if defined(DEBUG_DOH)
#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>
static int callCounter = 0;
#endif /* defined(DEBUG_DOH) */


static QString getFormattedWeight(const struct dive *dive, int idx)
{
	const weightsystem_t *weight = &dive->weightsystems.weightsystems[idx];
	if (!weight->description)
		return QString();
	QString fmt = QString(weight->description);
	fmt += ", " + get_weight_string(weight->weight, true);
	return fmt;
}

static QString getFormattedCylinder(const struct dive *dive, int idx)
{
	const cylinder_t *cyl = get_cylinder(dive, idx);
	const char *desc = cyl->type.description;
	QString fmt = desc ? QString(desc) : gettextFromC::tr("unknown");
	fmt += ", " + get_volume_string(cyl->type.size, true);
	fmt += ", " + get_pressure_string(cyl->type.workingpressure, true);
	fmt += ", " + get_pressure_string(cyl->start, false) + " - " + get_pressure_string(cyl->end, true);
	fmt += ", " + get_gas_string(cyl->gasmix);
	return fmt;
}

static QString formatGas(const dive *d)
{
	/*WARNING: here should be the gastlist, returned
	 * from the get_gas_string function or this is correct?
	 */
	QString gas, gases;
	for (int i = 0; i < d->cylinders.nr; i++) {
		if (!is_cylinder_used(d, i))
			continue;
		gas = get_cylinder(d, i)->type.description;
		if (!gas.isEmpty())
			gas += QChar(' ');
		gas += gasname(get_cylinder(d, i)->gasmix);
		// if has a description and if such gas is not already present
		if (!gas.isEmpty() && gases.indexOf(gas) == -1) {
			if (!gases.isEmpty())
				gases += QString(" / ");
			gases += gas;
		}
	}
	return gases;
}

static QString formatWeightList(const dive *d)
{
	QString weights;
	for (int i = 0; i < d->weightsystems.nr; i++) {
		QString w = getFormattedWeight(d, i);
		if (w.isEmpty())
			continue;
		weights += w + "; ";
	}
	return weights;
}

static QStringList formatWeights(const dive *d)
{
	QStringList weights;
	for (int i = 0; i < d->weightsystems.nr; i++) {
		QString w = getFormattedWeight(d, i);
		if (w.isEmpty())
			continue;
		weights << w;
	}
	return weights;
}

QStringList formatCylinders(const dive *d)
{
	QStringList cylinders;
	for (int i = 0; i < d->cylinders.nr; i++) {
		QString cyl = getFormattedCylinder(d, i);
		cylinders << cyl;
	}
	return cylinders;
}

static QVector<CylinderObjectHelper> makeCylinderObjects(const dive *d)
{
	QVector<CylinderObjectHelper> res;
	for (int i = 0; i < d->cylinders.nr; i++) {
		//Don't add blank cylinders, only those that have been defined.
		if (get_cylinder(d, i)->type.description)
			res.append(CylinderObjectHelper(get_cylinder(d, i))); // no emplace for QVector. :(
	}
	return res;
}

QString formatDiveSalinity(const dive *d)
{
	int salinity = get_dive_salinity(d);
	if (!salinity)
		return QString();
	return get_salinity_string(salinity);
}

QString formatDiveWaterType(const dive *d)
{
	return get_water_type_string(get_dive_salinity(d));
}

// Qt's metatype system insists on generating a default constructed object, even if that makes no sense.
DiveObjectHelper::DiveObjectHelper()
{
}

DiveObjectHelper::DiveObjectHelper(const struct dive *d) :
	number(d->number),
	id(d->id),
	rating(d->rating),
	visibility(d->visibility),
	wavesize(d->wavesize),
	current(d->current),
	surge(d->surge),
	chill(d->chill),
	timestamp(d->when),
	location(get_dive_location(d) ? QString::fromUtf8(get_dive_location(d)) : QString()),
	gps(d->dive_site ? printGPSCoords(&d->dive_site->location) : QString()),
	gps_decimal(format_gps_decimal(d)),
	dive_site(QVariant::fromValue(d->dive_site)),
	duration(get_dive_duration_string(d->duration.seconds, gettextFromC::tr("h"), gettextFromC::tr("min"))),
	noDive(d->duration.seconds == 0 && d->dc.duration.seconds == 0),
	depth(get_depth_string(d->dc.maxdepth.mm, true, true)),
	divemaster(d->divemaster ? d->divemaster : QString()),
	buddy(d->buddy ? d->buddy : QString()),
	airTemp(get_temperature_string(d->airtemp, true)),
	waterTemp(get_temperature_string(d->watertemp, true)),
	notes(formatNotes(d)),
	tags(get_taglist_string(d->tag_list)),
	gas(formatGas(d)),
	sac(formatSac(d)),
	weightList(formatWeightList(d)),
	weights(formatWeights(d)),
	singleWeight(d->weightsystems.nr <= 1),
	suit(d->suit ? d->suit : QString()),
	cylinders(formatCylinders(d)),
	maxcns(d->maxcns),
	otu(d->otu),
	sumWeight(get_weight_string(weight_t { total_weight(d) }, true)),
	getCylinder(formatGetCylinder(d)),
	startPressure(formatStartPressure(d)),
	endPressure(formatEndPressure(d)),
	firstGas(formatFirstGas(d)),
	salinity(formatDiveSalinity(d)),
	waterType(formatDiveWaterType(d))
{
#if defined(DEBUG_DOH)
	void *array[4];
	size_t size;

	// get void*'s for all entries on the stack
	size = backtrace(array, 4);

	// print out all the frames to stderr
	fprintf(stderr, "\n\nCalling DiveObjectHelper constructor for dive %d - call #%d\n", d->number, ++callCounter);
	backtrace_symbols_fd(array, size, STDERR_FILENO);
#endif /* defined(DEBUG_DOH) */
}

DiveObjectHelperGrantlee::DiveObjectHelperGrantlee()
{
}

DiveObjectHelperGrantlee::DiveObjectHelperGrantlee(const struct dive *d) :
	DiveObjectHelper(d),
	cylinderObjects(makeCylinderObjects(d))
{
}

QString DiveObjectHelper::date() const
{
	QDateTime localTime = timestampToDateTime(timestamp);
	return localTime.date().toString(prefs.date_format_short);
}

QString DiveObjectHelper::time() const
{
	QDateTime localTime = timestampToDateTime(timestamp);
	return localTime.time().toString(prefs.time_format);
}

QStringList DiveObjectHelper::cylinderList() const
{
	return formatFullCylinderList();
}
