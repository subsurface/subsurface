#include "string-format.h"
#include "dive.h"
#include "divesite.h"
#include "qthelper.h"
#include "subsurface-string.h"
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
