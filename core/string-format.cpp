#include "string-format.h"
#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "divesite.h"
#include "event.h"
#include "format.h"
#include "gettextfromc.h"
#include "pref.h"
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
				gases += QStringLiteral(" / ");
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
	QDateTime localTime = timestampToDateTime(d->get_time_local());
	return localTime.date().toString(QString::fromStdString(prefs.date_format_short));
}

QString formatDiveTime(const dive *d)
{
	QDateTime localTime = timestampToDateTime(d->get_time_local());
	return localTime.time().toString(QString::fromStdString(prefs.time_format));
}

QString formatDiveDateTime(const dive *d)
{
	QDateTime localTime = timestampToDateTime(d->get_time_local());
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

QString get_utc_offset_string(std::optional<int> offset)
{
	if (!offset)
		return QString();
	if (*offset == 0)
		return QStringLiteral("0:00");
	return qasprintf_loc("%+d:%02d", *offset / 3600, std::abs(*offset / 60) % 60);
}

QString get_depth_string(int mm, bool showunit, bool showdecimal)
{
	if (prefs.units.length == units::METERS) {
		double meters = mm / 1000.0;
		return QStringLiteral("%L1%2").arg(meters, 0, 'f', (showdecimal && meters < 20.0) ? 1 : 0).arg(showunit ? gettextFromC::tr("m") : QString());
	} else {
		double feet = mm_to_feet(mm);
		return QStringLiteral("%L1%2").arg(feet, 0, 'f', 0).arg(showunit ? gettextFromC::tr("ft") : QString());
	}
}

QString get_depth_string(depth_t depth, bool showunit, bool showdecimal)
{
	return get_depth_string(depth.mm, showunit, showdecimal);
}

QString get_depth_unit(bool metric)
{
	if (metric)
		return gettextFromC::tr("m");
	else
		return gettextFromC::tr("ft");
}

QString get_depth_unit()
{
	return get_depth_unit(prefs.units.length == units::METERS);
}

static QString weight_string(weight_t weight)
{
	QString str;
	if (get_units()->weight == units::KG) {
		double kg = (double) weight.grams / 1000.0;
		str = QStringLiteral("%L1").arg(kg, 0, 'f', kg >= 20.0 ? 0 : 1);
	} else {
		double lbs = grams_to_lbs(weight.grams);
		str = QStringLiteral("%L1").arg(lbs, 0, 'f', lbs >= 40.0 ? 0 : 1);
	}
	return str;
}

QString get_weight_string(weight_t weight, bool showunit)
{
	QString str = weight_string(weight);
	return showunit ? str + get_weight_unit() : str;
}

QString get_weight_unit(bool metric)
{
	if (metric)
		return gettextFromC::tr("kg");
	else
		return gettextFromC::tr("lbs");
}

QString get_weight_unit()
{
	return get_weight_unit(prefs.units.weight == units::KG);
}

QString get_temperature_string(temperature_t temp, bool showunit)
{
	if (temp.mkelvin == 0) {
		return ""; //temperature not defined
	} else if (prefs.units.temperature == units::CELSIUS) {
		double celsius = mkelvin_to_C(temp.mkelvin);
		return QStringLiteral("%L1%2%3").arg(celsius, 0, 'f', 1).arg(showunit ? "°" : "").arg(showunit ? gettextFromC::tr("C") : QString());
	} else {
		double fahrenheit = mkelvin_to_F(temp.mkelvin);
		return QStringLiteral("%L1%2%3").arg(fahrenheit, 0, 'f', 1).arg(showunit ? "°" : "").arg(showunit ? gettextFromC::tr("F") : QString());
	}
}

QString get_temp_unit(bool metric)
{
	if (metric)
		return QStringLiteral("°C");
	else
		return QStringLiteral("°F");
}

QString get_temp_unit()
{
	return get_temp_unit(prefs.units.temperature == units::CELSIUS);
}

QString get_volume_string(int mliter, bool showunit)
{
	const char *unit;
	int decimals;
	double value = get_volume_units(mliter, &decimals, &unit);
	return QStringLiteral("%L1%2").arg(value, 0, 'f', decimals).arg(showunit ? unit : "");
}

QString get_volume_string(volume_t volume, bool showunit)
{
	return get_volume_string(volume.mliter, showunit);
}

QString get_volume_unit(bool metric)
{
	if (metric)
		return gettextFromC::tr("L");
	else
		return gettextFromC::tr("cuft");
}

QString get_volume_unit()
{
	return get_volume_unit(prefs.units.volume == units::LITER);
}

QString get_pressure_string(pressure_t pressure, bool showunit)
{
	if (prefs.units.pressure == units::BAR) {
		double bar = pressure.mbar / 1000.0;
		return QStringLiteral("%L1%2").arg(bar, 0, 'f', 0).arg(showunit ? gettextFromC::tr("bar") : QString());
	} else {
		double psi = mbar_to_PSI(pressure.mbar);
		return QStringLiteral("%L1%2").arg(psi, 0, 'f', 0).arg(showunit ? gettextFromC::tr("psi") : QString());
	}
}

QString get_salinity_string(int salinity)
{
	return QStringLiteral("%L1%2").arg(salinity / 10.0).arg(gettextFromC::tr("g/L"));
}

// the water types need to match the watertypes enum
static const char *waterTypes[] = {
	QT_TRANSLATE_NOOP("gettextFromC", "Fresh"),
	QT_TRANSLATE_NOOP("gettextFromC", "Brackish"),
	QT_TRANSLATE_NOOP("gettextFromC", "EN13319"),
	QT_TRANSLATE_NOOP("gettextFromC", "Salt"),
	QT_TRANSLATE_NOOP("gettextFromC", "Use DC")
};

QString get_water_type_string(int salinity)
{
	if (salinity < 10050)
		return gettextFromC::tr(waterTypes[FRESHWATER]);
	else if (salinity < 10190)
		return gettextFromC::tr(waterTypes[BRACKISHWATER]);
	else if (salinity < 10210)
		return gettextFromC::tr(waterTypes[EN13319WATER]);
	else
		return gettextFromC::tr(waterTypes[SALTWATER]);
}

QStringList get_water_types_as_string()
{
	QStringList res;
	res.reserve(std::size(waterTypes));
	for (const char *t: waterTypes)
		res.push_back(gettextFromC::tr(t));
	return res;
}

QString get_dive_duration_string(timestamp_t when, QString hoursText, QString minutesText, QString secondsText, QString separator, bool isFreeDive)
{
	int hrs, mins, fullmins, secs;
	mins = (when + 30) / 60;
	fullmins = when / 60;
	secs = when - 60 * fullmins;
	hrs = mins / 60;

	QString displayTime;
	if (prefs.units.duration_units == units::ALWAYS_HOURS || (prefs.units.duration_units == units::MIXED && hrs)) {
		mins -= hrs * 60;
		displayTime = QStringLiteral("%1%2%3%4%5").arg(hrs).arg(separator == ":" ? "" : hoursText).arg(separator)
			.arg(mins, 2, 10, QChar('0')).arg(separator == ":" ? hoursText : minutesText);
	} else if (isFreeDive && ( prefs.units.duration_units == units::MINUTES_ONLY || minutesText != "" )) {
		// Freedive <1h and we display no hours but only minutes for other dives
		// --> display a short (5min 35sec) freedives e.g. as "5:35"
		// Freedive <1h and we display a unit for minutes
		// --> display a short (5min 35sec) freedives e.g. as "5:35min"
		if (separator == ":") displayTime = QStringLiteral("%1%2%3%4").arg(fullmins).arg(separator)
			.arg(secs, 2, 10, QChar('0')).arg(minutesText);
		else displayTime = QStringLiteral("%1%2%3%4%5").arg(fullmins).arg(minutesText).arg(separator)
			.arg(secs).arg(secondsText);
	} else if (isFreeDive) {
		// Mixed display (hh:mm / mm only) and freedive < 1h and we have no unit for minutes
		// --> Prefix duration with "0:" --> "0:05:35"
		if (separator == ":") displayTime = QStringLiteral("%1%2%3%4%5%6").arg(hrs).arg(separator)
			.arg(fullmins, 2, 10, QChar('0')).arg(separator)
			.arg(secs, 2, 10, QChar('0')).arg(hoursText);
		// Separator != ":" and no units for minutes --> unlikely case - remove?
		else displayTime = QStringLiteral("%1%2%3%4%5%6%7%8").arg(hrs).arg(hoursText).arg(separator)
			.arg(fullmins).arg(minutesText).arg(separator)
			.arg(secs).arg(secondsText);
	} else {
		displayTime = QStringLiteral("%1%2").arg(mins).arg(minutesText);
	}
	return displayTime;
}

QString get_dive_duration_string(timestamp_t when, QString hoursText, QString minutesText)
{
	return get_dive_duration_string(when, hoursText, minutesText, gettextFromC::tr("sec"), QStringLiteral(":"), false);
}

QString get_dive_surfint_string(timestamp_t when, QString daysText, QString hoursText, QString minutesText, QString separator, int maxdays)
{
	int days, hrs, mins;
	days = when / 3600 / 24;
	hrs = (when - days * 3600 * 24) / 3600;
	mins = (when + 30 - days * 3600 * 24 - hrs * 3600) / 60;

	QString displayInt;
	if (maxdays && days > maxdays) displayInt = gettextFromC::tr("more than %1 days").arg(maxdays);
	else if (days) displayInt = QStringLiteral("%1%2%3%4%5%6%7%8").arg(days).arg(daysText).arg(separator)
		.arg(hrs).arg(hoursText).arg(separator)
		.arg(mins).arg(minutesText);
	else displayInt = QStringLiteral("%1%2%3%4%5").arg(hrs).arg(hoursText).arg(separator)
		.arg(mins).arg(minutesText);
	return displayInt;
}

QString get_dive_date_string(timestamp_t when)
{
	QDateTime ts;
	ts.setMSecsSinceEpoch(when * 1000L);
	return loc.toString(ts.toUTC(), QString::fromStdString(prefs.date_format + " " + prefs.time_format));
}

std::string get_dive_date_c_string(timestamp_t when)
{
	return get_short_dive_date_string(when).toStdString();
}

QString distance_string(int distanceInMeters)
{
	QString str;
	if(get_units()->length == units::METERS) {
		if (distanceInMeters >= 1000)
			str = gettextFromC::tr("%1km").arg(distanceInMeters / 1000);
		else
			str = gettextFromC::tr("%1m").arg(distanceInMeters);
	} else {
		double miles = m_to_mile(distanceInMeters);
		if (miles >= 1.0)
			str = gettextFromC::tr("%1mi").arg((int)miles);
		else
			str = gettextFromC::tr("%1yd").arg((int)(miles * 1760));
	}
	return str;
}

QString printGPSCoords(const location_t *location)
{
	int lat = location->lat.udeg;
	int lon = location->lon.udeg;
	unsigned int latdeg, londeg;
	unsigned int latmin, lonmin;
	double latsec, lonsec;
	QString lath, lonh, result;

	if (!has_location(location))
		return QString();

	if (prefs.coordinates_traditional) {
		lath = lat >= 0 ? gettextFromC::tr("N") : gettextFromC::tr("S");
		lonh = lon >= 0 ? gettextFromC::tr("E") : gettextFromC::tr("W");
		lat = abs(lat);
		lon = abs(lon);
		latdeg = lat / 1000000U;
		londeg = lon / 1000000U;
		latmin = (lat % 1000000U) * 60U;
		lonmin = (lon % 1000000U) * 60U;
		latsec = (latmin % 1000000) * 60;
		lonsec = (lonmin % 1000000) * 60;
		result = QString::asprintf("%u°%02d\'%06.3f\"%s %u°%02d\'%06.3f\"%s",
			       latdeg, latmin / 1000000, latsec / 1000000, qPrintable(lath),
			       londeg, lonmin / 1000000, lonsec / 1000000, qPrintable(lonh));
	} else {
		result = QString::asprintf("%f %f", (double) lat / 1000000.0, (double) lon / 1000000.0);
	}
	return result;
}

std::string printGPSCoordsC(const location_t *location)
{
	return printGPSCoords(location).toStdString();
}
