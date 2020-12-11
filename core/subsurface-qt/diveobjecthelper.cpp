// SPDX-License-Identifier: GPL-2.0
#include "diveobjecthelper.h"

#include <QDateTime>
#include <QTextDocument>

#include "core/dive.h"
#include "core/qthelper.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/subsurface-string.h"
#include "qt-models/tankinfomodel.h"

#if defined(DEBUG_DOH)
#include <execinfo.h>
#include <stdio.h>
#include <unistd.h>
static int callCounter = 0;
#endif /* defined(DEBUG_DOH) */


enum returnPressureSelector {START_PRESSURE, END_PRESSURE};

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

static QString getPressures(const struct dive *dive, int i, enum returnPressureSelector ret)
{
	const cylinder_t *cyl = get_cylinder(dive, i);
	QString fmt;
	if (ret == START_PRESSURE) {
		if (cyl->start.mbar)
			fmt = get_pressure_string(cyl->start, true);
		else if (cyl->sample_start.mbar)
			fmt = get_pressure_string(cyl->sample_start, true);
	}
	if (ret == END_PRESSURE) {
		if (cyl->end.mbar)
			fmt = get_pressure_string(cyl->end, true);
		else if(cyl->sample_end.mbar)
			fmt = get_pressure_string(cyl->sample_end, true);
	}
	return fmt;
}

QString format_gps_decimal(const dive *d)
{
	bool savep = prefs.coordinates_traditional;

	prefs.coordinates_traditional = false;
	QString val = d->dive_site ? printGPSCoords(&d->dive_site->location) : QString();
	prefs.coordinates_traditional = savep;
	return val;
}

QString formatNotes(const dive *d)
{
	QString tmp = d->notes ? QString::fromUtf8(d->notes) : QString();
	if (is_dc_planner(&d->dc)) {
		QTextDocument notes;
	#define _NOTES_BR "&#92n"
		tmp.replace("<thead>", "<thead>" _NOTES_BR)
			.replace("<br>", "<br>" _NOTES_BR)
			.replace("<br/>", "<br/>" _NOTES_BR)
			.replace("<br />", "<br />" _NOTES_BR)
			.replace("<tr>", "<tr>" _NOTES_BR)
			.replace("</tr>", "</tr>" _NOTES_BR);
		notes.setHtml(tmp);
		tmp = notes.toPlainText();
		tmp.replace(_NOTES_BR, "<br/>");
	#undef _NOTES_BR
	} else {
		tmp.replace("\n", "<br/>");
	}
	return tmp;
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

QString formatSac(const dive *d)
{
	if (!d->sac)
		return QString();
	const char *unit;
	int decimal;
	double value = get_volume_units(d->sac, &decimal, &unit);
	return QString::number(value, 'f', decimal).append(unit);
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

QStringList formatGetCylinder(const dive *d)
{
	QStringList getCylinder;
	for (int i = 0; i < d->cylinders.nr; i++) {
		if (is_cylinder_used(d, i))
			getCylinder << get_cylinder(d, i)->type.description;
	}
	return getCylinder;
}

QStringList getStartPressure(const dive *d)
{
	QStringList startPressure;
	for (int i = 0; i < d->cylinders.nr; i++) {
		if (is_cylinder_used(d, i))
			startPressure << getPressures(d, i, START_PRESSURE);
	}
	return startPressure;
}

QStringList getEndPressure(const dive *d)
{
	QStringList endPressure;
	for (int i = 0; i < d->cylinders.nr; i++) {
		if (is_cylinder_used(d, i))
			endPressure << getPressures(d, i, END_PRESSURE);
	}
	return endPressure;
}

QStringList getFirstGas(const dive *d)
{
	QStringList gas;
	for (int i = 0; i < d->cylinders.nr; i++) {
		if (is_cylinder_used(d, i))
			gas << get_gas_string(get_cylinder(d, i)->gasmix);
	}
	return gas;
}

// Add string to sorted QStringList, if it doesn't already exist and
// it isn't the empty string.
static void addStringToSortedList(QStringList &l, const char *s)
{
	if (empty_string(s))
		return;

	// Do a binary search for the string. lower_bound() returns an iterator
	// to either the searched-for element or the next higher element if it
	// doesn't exist.
	QString qs(s);
	auto it = std::lower_bound(l.begin(), l.end(), qs); // TODO: use locale-aware sorting
	if (it != l.end() && *it == s)
		return;

	// Add new string at sorted position
	l.insert(it, s);
}

QStringList getFullCylinderList()
{
	QStringList cylinders;
	struct dive *d;
	int i = 0;
	for_each_dive (i, d) {
		for (int j = 0; j < d->cylinders.nr; j++)
			addStringToSortedList(cylinders, get_cylinder(d, j)->type.description);
	}

	for (int ti = 0; ti < tank_info_table.nr; ti++)
		addStringToSortedList(cylinders, tank_info_table.infos[ti].name);

	return cylinders;
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
	startPressure(getStartPressure(d)),
	endPressure(getEndPressure(d)),
	firstGas(getFirstGas(d)),
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
	return getFullCylinderList();
}
