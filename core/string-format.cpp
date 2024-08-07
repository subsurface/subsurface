#include "string-format.h"
#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "divesite.h"
#include "event.h"
#include "format.h"
#include "qthelper.h"
#include "range.h"
#include "subsurface-string.h"
#include "trip.h"
#include <QDateTime>
#include <QLocale>
#include <QTextDocument>

enum returnPressureSelector { START_PRESSURE, END_PRESSURE };
static QLocale loc;

static QString getPressures(const cylinder_t &cyl, enum returnPressureSelector ret)
{
	if (ret == START_PRESSURE) {
		if (cyl.start.mbar)
			return get_pressure_string(cyl.start, true);
		else if (cyl.sample_start.mbar)
			return get_pressure_string(cyl.sample_start, true);
	}
	if (ret == END_PRESSURE) {
		if (cyl.end.mbar)
			return get_pressure_string(cyl.end, true);
		else if (cyl.sample_end.mbar)
			return get_pressure_string(cyl.sample_end, true);
	}
	return QString();
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
	QString tmp = QString::fromStdString(d->notes);
	if (is_dc_planner(&d->dcs[0])) {
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
	for (auto [i, cyl]: enumerated_range(d->cylinders)) {
		if (d->is_cylinder_used(i))
			getCylinder << QString::fromStdString(cyl.type.description);
	}
	return getCylinder;
}

QStringList formatStartPressure(const dive *d)
{
	QStringList startPressure;
	for (auto [i, cyl]: enumerated_range(d->cylinders)) {
		if (d->is_cylinder_used(i))
			startPressure << getPressures(cyl, START_PRESSURE);
	}
	return startPressure;
}

QStringList formatEndPressure(const dive *d)
{
	QStringList endPressure;
	for (auto [i, cyl]: enumerated_range(d->cylinders)) {
		if (d->is_cylinder_used(i))
			endPressure << getPressures(cyl, END_PRESSURE);
	}
	return endPressure;
}

QStringList formatFirstGas(const dive *d)
{
	QStringList gas;
	for (auto [i, cyl]: enumerated_range(d->cylinders)) {
		if (d->is_cylinder_used(i))
			gas << get_gas_string(cyl.gasmix);
	}
	return gas;
}

// Add string to sorted QStringList, if it doesn't already exist and
// it isn't the empty string.
static void addStringToSortedList(QStringList &l, const std::string &s)
{
	if (s.empty())
		return;

	// Do a binary search for the string. lower_bound() returns an iterator
	// to either the searched-for element or the next higher element if it
	// doesn't exist.
	QString qs = QString::fromStdString(s);
	auto it = std::lower_bound(l.begin(), l.end(), qs); // TODO: use locale-aware sorting
	if (it != l.end() && *it == qs)
		return;

	// Add new string at sorted position
	l.insert(it, qs);
}

QStringList formatFullCylinderList()
{
	QStringList cylinders;
	for (auto &d: divelog.dives) {
		for (const cylinder_t &cyl: d->cylinders)
			addStringToSortedList(cylinders, cyl.type.description);
	}

	for (const auto &ti: tank_info_table)
		addStringToSortedList(cylinders, ti.name);

	return cylinders;
}

static QString formattedCylinder(const cylinder_t &cyl)
{
	const std::string &desc = cyl.type.description;
	QString fmt = !desc.empty() ? QString::fromStdString(desc) : gettextFromC::tr("unknown");
	fmt += ", " + get_volume_string(cyl.type.size, true);
	fmt += ", " + get_pressure_string(cyl.type.workingpressure, true);
	fmt += ", " + get_pressure_string(cyl.start, false) + " - " + get_pressure_string(cyl.end, true);
	fmt += ", " + get_gas_string(cyl.gasmix);
	return fmt;
}

QStringList formatCylinders(const dive *d)
{
	QStringList cylinders;
	for (const cylinder_t &cyl: d->cylinders)
		cylinders << formattedCylinder(cyl);
	return cylinders;
}

QString formatGas(const dive *d)
{
	/*WARNING: here should be the gastlist, returned
	 * from the get_gas_string function or this is correct?
	 */
	QString gases;
	for (auto [i, cyl]: enumerated_range(d->cylinders)) {
		if (!d->is_cylinder_used(i))
			continue;
		QString gas = QString::fromStdString(cyl.type.description);
		if (!gas.isEmpty())
			gas += QChar(' ');
		gas += QString::fromStdString(cyl.gasmix.name());
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
	return get_weight_string(d->total_weight(), true);
}

static QString getFormattedWeight(const weightsystem_t &weight)
{
	if (weight.description.empty())
		return QString();
	return QString::fromStdString(weight.description) +
	       ", " + get_weight_string(weight.weight, true);
}

QString formatWeightList(const dive *d)
{
	QString weights;
	for (auto &ws: d->weightsystems) {
		QString w = getFormattedWeight(ws);
		if (w.isEmpty())
			continue;
		weights += w + "; ";
	}
	return weights;
}

QStringList formatWeights(const dive *d)
{
	QStringList weights;
	for (auto &ws: d->weightsystems) {
		QString w = getFormattedWeight(ws);
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
	return localTime.date().toString(QString::fromStdString(prefs.date_format_short));
}

QString formatDiveTime(const dive *d)
{
	QDateTime localTime = timestampToDateTime(d->when);
	return localTime.time().toString(QString::fromStdString(prefs.time_format));
}

QString formatDiveDateTime(const dive *d)
{
	QDateTime localTime = timestampToDateTime(d->when);
	return QStringLiteral("%1 %2").arg(localTime.date().toString(QString::fromStdString(prefs.date_format_short)),
					   localTime.time().toString(QString::fromStdString(prefs.time_format)));
}

QString formatDiveGasString(const dive *d)
{
	auto [o2, he, o2max ] = d->get_maximal_gas();
	o2 = (o2 + 5) / 10;
	he = (he + 5) / 10;
	o2max = (o2max + 5) / 10;

	if (he) {
		if (o2 == o2max)
			return qasprintf_loc("%d/%d", o2, he);
		else
			return qasprintf_loc("%d/%d…%d%%", o2, he, o2max);
	} else if (o2) {
		if (o2 == o2max)
			return qasprintf_loc("%d%%", o2);
		else
			return qasprintf_loc("%d…%d%%", o2, o2max);
	} else {
		return gettextFromC::tr("air");
	}
}

QString formatDayOfWeek(int day)
{
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

QString formatMinutes(int seconds)
{
	return QString::asprintf("%d:%.2d", FRACTION_TUPLE(seconds, 60));
}

QString formatTripTitle(const dive_trip &trip)
{
	timestamp_t when = trip.date();
	bool getday = trip.is_single_day();

	QDateTime localTime = timestampToDateTime(when);

	QString prefix = !trip.location.empty() ? QString::fromStdString(trip.location) + ", " : QString();
	if (getday)
		return prefix + loc.toString(localTime, QString::fromStdString(prefs.date_format));
	else
		return prefix + loc.toString(localTime, "MMM yyyy");
}

QString formatTripTitleWithDives(const dive_trip &trip)
{
	int nr = static_cast<int>(trip.dives.size());
	return formatTripTitle(trip) + " " +
	       gettextFromC::tr("(%n dive(s))", "", nr);
}
