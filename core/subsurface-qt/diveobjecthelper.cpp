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
	location(get_dive_location(d)),
	gps(formatDiveGPS(d)),
	gps_decimal(format_gps_decimal(d)),
	dive_site(QVariant::fromValue(d->dive_site)),
	duration(formatDiveDuration(d)),
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
	sumWeight(formatSumWeight(d)),
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
