// SPDX-License-Identifier: GPL-2.0
#include "DiveObjectHelper.h"

#include <QDateTime>
#include <QTextDocument>

#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include "qt-models/tankinfomodel.h"

enum returnPressureSelector {START_PRESSURE, END_PRESSURE};

static QString getFormattedWeight(struct dive *dive, unsigned int idx)
{
	weightsystem_t *weight = &dive->weightsystem[idx];
	if (!weight->description)
		return QString();
	QString fmt = QString(weight->description);
	fmt += ", " + get_weight_string(weight->weight, true);
	return fmt;
}

static QString getFormattedCylinder(struct dive *dive, unsigned int idx)
{
	cylinder_t *cyl = &dive->cylinder[idx];
	const char *desc = cyl->type.description;
	if (!desc && idx > 0)
		return QString();
	QString fmt = desc ? QString(desc) : gettextFromC::tr("unknown");
	fmt += ", " + get_volume_string(cyl->type.size, true);
	fmt += ", " + get_pressure_string(cyl->type.workingpressure, true);
	fmt += ", " + get_pressure_string(cyl->start, false) + " - " + get_pressure_string(cyl->end, true);
	fmt += ", " + get_gas_string(cyl->gasmix);
	return fmt;
}

static QString getPressures(struct dive *dive, int i, enum returnPressureSelector ret)
{
	cylinder_t *cyl = &dive->cylinder[i];
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

DiveObjectHelper::DiveObjectHelper(struct dive *d) :
	m_dive(d)
{
	m_cyls.clear();
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		//Don't add blank cylinders, only those that have been defined.
		if (m_dive->cylinder[i].type.description)
			m_cyls.append(new CylinderObjectHelper(&m_dive->cylinder[i]));
	}
}

DiveObjectHelper::~DiveObjectHelper()
{
while (!m_cyls.isEmpty())
	delete m_cyls.takeFirst();
}

int DiveObjectHelper::number() const
{
	return m_dive->number;
}

int DiveObjectHelper::id() const
{
	return m_dive->id;
}

struct dive *DiveObjectHelper::getDive() const
{
	return m_dive;
}

QString DiveObjectHelper::date() const
{
	QDateTime localTime = QDateTime::fromMSecsSinceEpoch(1000*m_dive->when, Qt::UTC);
	localTime.setTimeSpec(Qt::UTC);
	return localTime.date().toString(prefs.date_format_short);
}

timestamp_t DiveObjectHelper::timestamp() const
{
	return m_dive->when;
}

QString DiveObjectHelper::time() const
{
	QDateTime localTime = QDateTime::fromMSecsSinceEpoch(1000*m_dive->when, Qt::UTC);
	localTime.setTimeSpec(Qt::UTC);
	return localTime.time().toString(prefs.time_format);
}

QString DiveObjectHelper::location() const
{
	return get_dive_location(m_dive) ? QString::fromUtf8(get_dive_location(m_dive)) : QString();
}

QString DiveObjectHelper::gps() const
{
	struct dive_site *ds = m_dive->dive_site;
	if (!ds)
		return QString();
	char *gps = printGPSCoords(&ds->location);
	QString res = gps;
	free(gps);
	return res;
}

QString DiveObjectHelper::gps_decimal() const
{
	bool savep = prefs.coordinates_traditional;
	QString val;

	prefs.coordinates_traditional = false;
	val = gps();
	prefs.coordinates_traditional = savep;
	return val;
}

QVariant DiveObjectHelper::dive_site() const
{
	return QVariant::fromValue(m_dive->dive_site);
}

QString DiveObjectHelper::duration() const
{
	return get_dive_duration_string(m_dive->duration.seconds, gettextFromC::tr("h"), gettextFromC::tr("min"));
}

bool DiveObjectHelper::noDive() const
{
	return m_dive->duration.seconds == 0 && m_dive->dc.duration.seconds == 0;
}

QString DiveObjectHelper::depth() const
{
	return get_depth_string(m_dive->dc.maxdepth.mm, true, true);
}

QString DiveObjectHelper::divemaster() const
{
	return m_dive->divemaster ? m_dive->divemaster : QString();
}

QString DiveObjectHelper::buddy() const
{
	return m_dive->buddy ? m_dive->buddy : QString();
}

QString DiveObjectHelper::airTemp() const
{
	return get_temperature_string(m_dive->airtemp, true);
}

QString DiveObjectHelper::waterTemp() const
{
	return get_temperature_string(m_dive->watertemp, true);
}

QString DiveObjectHelper::notes() const
{
	QString tmp = m_dive->notes ? QString::fromUtf8(m_dive->notes) : QString();
	if (is_dc_planner(&m_dive->dc)) {
		QTextDocument notes;
	#define _NOTES_BR "&#92n"
		tmp.replace("<thead>", "<thead>" _NOTES_BR)
			.replace("<br>", "<br>" _NOTES_BR)
			.replace("<tr>", "<tr>" _NOTES_BR)
			.replace("</tr>", "</tr>" _NOTES_BR);
		notes.setHtml(tmp);
		tmp = notes.toPlainText();
		tmp.replace(_NOTES_BR, "<br>");
	#undef _NOTES_BR
	} else {
		tmp.replace("\n", "<br>");
	}
	return tmp;
}

QString DiveObjectHelper::tags() const
{
	return get_taglist_string(m_dive->tag_list);
}

QString DiveObjectHelper::gas() const
{
	/*WARNING: here should be the gastlist, returned
	 * from the get_gas_string function or this is correct?
	 */
	QString gas, gases;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (!is_cylinder_used(m_dive, i))
			continue;
		gas = m_dive->cylinder[i].type.description;
		if (!gas.isEmpty())
			gas += QChar(' ');
		gas += gasname(m_dive->cylinder[i].gasmix);
		// if has a description and if such gas is not already present
		if (!gas.isEmpty() && gases.indexOf(gas) == -1) {
			if (!gases.isEmpty())
				gases += QString(" / ");
			gases += gas;
		}
	}
	return gases;
}

QString DiveObjectHelper::sac() const
{
	if (!m_dive->sac)
		return QString();
	const char *unit;
	int decimal;
	double value = get_volume_units(m_dive->sac, &decimal, &unit);
	return QString::number(value, 'f', decimal).append(unit);
}

QString DiveObjectHelper::weightList() const
{
	QString weights;
	for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
		QString w = getFormattedWeight(m_dive, i);
		if (w.isEmpty())
			continue;
		weights += w + "; ";
	}
	return weights;
}

QStringList DiveObjectHelper::weights() const
{
	QStringList weights;
	for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++) {
		QString w = getFormattedWeight(m_dive, i);
		if (w.isEmpty())
			continue;
		weights << w;
	}
	return weights;
}

bool DiveObjectHelper::singleWeight() const
{
	return weightsystem_none(&m_dive->weightsystem[1]);
}

QString DiveObjectHelper::weight(int idx) const
{
	if ( (idx < 0) || idx > MAX_WEIGHTSYSTEMS )
		return QString();
	return getFormattedWeight(m_dive, idx);
}

QString DiveObjectHelper::suit() const
{
	return m_dive->suit ? m_dive->suit : QString();
}

QStringList DiveObjectHelper::cylinderList() const
{
	QStringList cylinders;
	struct dive *d;
	int i = 0;
	for_each_dive (i, d) {
		for (int j = 0; j < MAX_CYLINDERS; j++) {
			QString cyl = d->cylinder[j].type.description;
			if (cyl.isEmpty())
				continue;
			cylinders << cyl;
		}
	}

	for (unsigned long ti = 0; ti < MAX_TANK_INFO && tank_info[ti].name != NULL; ti++) {
		QString cyl = tank_info[ti].name;
		if (cyl.isEmpty())
			continue;
		cylinders << cyl;
	}

	cylinders.removeDuplicates();
	cylinders.sort();
	return cylinders;
}

QStringList DiveObjectHelper::cylinders() const
{
	QStringList cylinders;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		QString cyl = getFormattedCylinder(m_dive, i);
		if (cyl.isEmpty())
			continue;
		cylinders << cyl;
	}
	return cylinders;
}

QString DiveObjectHelper::cylinder(int idx) const
{
	if ( (idx < 0) || idx > MAX_CYLINDERS)
		return QString();
	return getFormattedCylinder(m_dive, idx);
}

QList<CylinderObjectHelper*> DiveObjectHelper::cylinderObjects() const
{
	return m_cyls;
}

QString DiveObjectHelper::tripId() const
{
	return m_dive->divetrip ? QString::number((quint64)m_dive->divetrip, 16) : QString();
}

int DiveObjectHelper::tripNrDives() const
{
	struct dive_trip *dt = m_dive->divetrip;
	return dt ? dt->dives.nr : 0;
}

int DiveObjectHelper::maxcns() const
{
	return m_dive->maxcns;
}

int DiveObjectHelper::otu() const
{
	return m_dive->otu;
}

int DiveObjectHelper::rating() const
{
	return m_dive->rating;
}

int DiveObjectHelper::visibility() const
{
	return m_dive->visibility;
}

QString DiveObjectHelper::sumWeight() const
{
	weight_t sum = { 0 };
	for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++){
		sum.grams += m_dive->weightsystem[i].weight.grams;
	}
	return get_weight_string(sum, true);
}

QStringList DiveObjectHelper::getCylinder() const
{
	QStringList getCylinder; 
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (is_cylinder_used(m_dive, i))
			getCylinder << m_dive->cylinder[i].type.description;
	}
	return getCylinder;
}

QStringList DiveObjectHelper::startPressure() const
{
	QStringList startPressure;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (is_cylinder_used(m_dive, i))
			startPressure << getPressures(m_dive, i, START_PRESSURE);
	}
	return startPressure;
}

QStringList DiveObjectHelper::endPressure() const
{
	QStringList endPressure;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (is_cylinder_used(m_dive, i))
			endPressure << getPressures(m_dive, i, END_PRESSURE);
	}
	return endPressure;
}

QStringList DiveObjectHelper::firstGas() const
{
	QStringList gas;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (is_cylinder_used(m_dive, i))
			gas << get_gas_string(m_dive->cylinder[i].gasmix);
	}
	return gas;
}

// for a full text search / filter function
QString DiveObjectHelper::fullText() const
{
	return fullTextNoNotes() + ":-:" + notes();
}

QString DiveObjectHelper::fullTextNoNotes() const
{
	QString tripLocation = m_dive->divetrip ? m_dive->divetrip->location : QString();
	return tripLocation + ":-:" + location() + ":-:" + buddy() + ":-:" + divemaster() + ":-:" + suit() + ":-:" + tags();
}
