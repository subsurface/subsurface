#include "string-format.h"
#include "dive.h"
#include "divesite.h"
#include "qthelper.h"
#include "subsurface-string.h"
#include <QDateTime>
#include <QTextDocument>

enum returnPressureSelector { START_PRESSURE, END_PRESSURE };

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

QString formatSac(const dive *d)
{
	if (!d->sac)
		return QString();
	const char *unit;
	int decimal;
	double value = get_volume_units(d->sac, &decimal, &unit);
	return QString::number(value, 'f', decimal).append(unit);
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

QString format_gps_decimal(const dive *d)
{
	bool savep = prefs.coordinates_traditional;

	prefs.coordinates_traditional = false;
	QString val = d->dive_site ? printGPSCoords(&d->dive_site->location) : QString();
	prefs.coordinates_traditional = savep;
	return val;
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

QStringList formatStartPressure(const dive *d)
{
	QStringList startPressure;
	for (int i = 0; i < d->cylinders.nr; i++) {
		if (is_cylinder_used(d, i))
			startPressure << getPressures(d, i, START_PRESSURE);
	}
	return startPressure;
}

QStringList formatEndPressure(const dive *d)
{
	QStringList endPressure;
	for (int i = 0; i < d->cylinders.nr; i++) {
		if (is_cylinder_used(d, i))
			endPressure << getPressures(d, i, END_PRESSURE);
	}
	return endPressure;
}

QStringList formatFirstGas(const dive *d)
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

QStringList formatFullCylinderList()
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

static QString formattedCylinder(const struct dive *dive, int idx)
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

QStringList formatCylinders(const dive *d)
{
	QStringList cylinders;
	for (int i = 0; i < d->cylinders.nr; i++) {
		QString cyl = formattedCylinder(d, i);
		cylinders << cyl;
	}
	return cylinders;
}

QString formatGas(const dive *d)
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

QString formatSumWeight(const dive *d)
{
	return get_weight_string(weight_t { total_weight(d) }, true);
}

static QString getFormattedWeight(const struct dive *dive, int idx)
{
	const weightsystem_t *weight = &dive->weightsystems.weightsystems[idx];
	if (!weight->description)
		return QString();
	QString fmt = QString(weight->description);
	fmt += ", " + get_weight_string(weight->weight, true);
	return fmt;
}

QString formatWeightList(const dive *d)
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

QStringList formatWeights(const dive *d)
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

QString formatDiveDuration(const dive *d)
{
	return get_dive_duration_string(d->duration.seconds,
					gettextFromC::tr("h"), gettextFromC::tr("min"));
}

QString formatDiveGPS(const dive *d)
{
	return d->dive_site ? printGPSCoords(&d->dive_site->location) : QString();
}

QString formatDiveDate(const dive *d)
{
	QDateTime localTime = timestampToDateTime(d->when);
	return localTime.date().toString(prefs.date_format_short);
}

QString formatDiveTime(const dive *d)
{
	QDateTime localTime = timestampToDateTime(d->when);
	return localTime.time().toString(prefs.time_format);
}

QString formatDiveDateTime(const dive *d)
{
	QDateTime localTime = timestampToDateTime(d->when);
	return QStringLiteral("%1 %2").arg(localTime.date().toString(prefs.date_format_short),
					   localTime.time().toString(prefs.time_format));
}

QString formatDayOfWeek(int day)
{
	// I can't wrap my head around the fact that Sunday is the
	// first day of the week, but that's how it is.
	switch (day) {
	default:
	case 0:	return gettextFromC::tr("Sunday");
	case 1:	return gettextFromC::tr("Monday");
	case 2:	return gettextFromC::tr("Tuesday");
	case 3:	return gettextFromC::tr("Wednesday");
	case 4:	return gettextFromC::tr("Thursday");
	case 5:	return gettextFromC::tr("Friday");
	case 6:	return gettextFromC::tr("Saturday");
	}
}
