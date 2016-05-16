#include "DiveObjectHelper.h"

#include <QDateTime>
#include <QTextDocument>

#include "../qthelper.h"
#include "../helpers.h"

static QString EMPTY_DIVE_STRING = QStringLiteral("");
enum returnPressureSelector {START_PRESSURE, END_PRESSURE};

static QString getFormattedWeight(struct dive *dive, unsigned int idx)
{
	weightsystem_t *weight = &dive->weightsystem[idx];
	if (!weight->description)
		return QString(EMPTY_DIVE_STRING);
	QString fmt = QString(weight->description);
	fmt += ", " + get_weight_string(weight->weight, true);
	return fmt;
}

static QString getFormattedCylinder(struct dive *dive, unsigned int idx)
{
	cylinder_t *cyl = &dive->cylinder[idx];
	const char *desc = cyl->type.description;
	if (!desc && idx > 0)
		return QString(EMPTY_DIVE_STRING);
	QString fmt = desc ? QString(desc) : QObject::tr("unknown");
	fmt += ", " + get_volume_string(cyl->type.size, true);
	fmt += ", " + get_pressure_string(cyl->type.workingpressure, true);
	fmt += ", " + get_pressure_string(cyl->start, false) + " - " + get_pressure_string(cyl->end, true);
	fmt += ", " + get_gas_string(cyl->gasmix);
	return fmt;
}

static QString getPressures(struct dive *dive, enum returnPressureSelector ret)
{
	cylinder_t *cyl = &dive->cylinder[0];
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
}

DiveObjectHelper::~DiveObjectHelper()
{
}

int DiveObjectHelper::number() const
{
	return m_dive->number;
}

int DiveObjectHelper::id() const
{
	return m_dive->id;
}

QString DiveObjectHelper::date() const
{
	QDateTime localTime = QDateTime::fromMSecsSinceEpoch(1000*m_dive->when, Qt::UTC);
	localTime.setTimeSpec(Qt::UTC);
	return localTime.date().toString(prefs.date_format);
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
	return get_dive_location(m_dive) ? QString::fromUtf8(get_dive_location(m_dive)) : EMPTY_DIVE_STRING;
}

QString DiveObjectHelper::gps() const
{
	struct dive_site *ds = get_dive_site_by_uuid(m_dive->dive_site_uuid);
	return ds ? QString(printGPSCoords(ds->latitude.udeg, ds->longitude.udeg)) : QString();
}
QString DiveObjectHelper::duration() const
{
	return get_dive_duration_string(m_dive->duration.seconds, QObject::tr("h:"), QObject::tr("min"));
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
	return m_dive->divemaster ? m_dive->divemaster : EMPTY_DIVE_STRING;
}

QString DiveObjectHelper::buddy() const
{
	return m_dive->buddy ? m_dive->buddy : EMPTY_DIVE_STRING;
}

QString DiveObjectHelper::airTemp() const
{
	QString temp = get_temperature_string(m_dive->airtemp, true);
	if (temp.isEmpty()) {
		temp = EMPTY_DIVE_STRING;
	}
	return temp;
}

QString DiveObjectHelper::waterTemp() const
{
	QString temp = get_temperature_string(m_dive->watertemp, true);
	if (temp.isEmpty()) {
		temp = EMPTY_DIVE_STRING;
	}
	return temp;
}

QString DiveObjectHelper::notes() const
{
	QString tmp = m_dive->notes ? QString::fromUtf8(m_dive->notes) : EMPTY_DIVE_STRING;
	if (same_string(m_dive->dc.model, "planned dive")) {
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
	static char buffer[256];
	taglist_get_tagstring(m_dive->tag_list, buffer, 256);
	return QString(buffer);
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
		gas += gasname(&m_dive->cylinder[i].gasmix);
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
		if (w == EMPTY_DIVE_STRING)
			continue;
		weights += w + "; ";
	}
	return weights;
}

QStringList DiveObjectHelper::weights() const
{
	QStringList weights;
	for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++)
		weights << getFormattedWeight(m_dive, i);
	return weights;
}

bool DiveObjectHelper::singleWeight() const
{
	return weightsystem_none(&m_dive->weightsystem[1]);
}

QString DiveObjectHelper::weight(int idx) const
{
	if ( (idx < 0) || idx > MAX_WEIGHTSYSTEMS )
		return QString(EMPTY_DIVE_STRING);
	return getFormattedWeight(m_dive, idx);
}

QString DiveObjectHelper::suit() const
{
	return m_dive->suit ? m_dive->suit : EMPTY_DIVE_STRING;
}

QString DiveObjectHelper::cylinderList() const
{
	QString cylinders;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		QString cyl = getFormattedCylinder(m_dive, i);
		if (cyl == EMPTY_DIVE_STRING)
			continue;
		cylinders += cyl + "; ";
	}
	return cylinders;
}

QStringList DiveObjectHelper::cylinders() const
{
	QStringList cylinders;
	for (int i = 0; i < MAX_CYLINDERS; i++)
		cylinders << getFormattedCylinder(m_dive, i);
	return cylinders;
}

QString DiveObjectHelper::cylinder(int idx) const
{
	if ( (idx < 0) || idx > MAX_CYLINDERS)
		return QString(EMPTY_DIVE_STRING);
	return getFormattedCylinder(m_dive, idx);
}

QString DiveObjectHelper::trip() const
{
	return m_dive->divetrip ? m_dive->divetrip->location : EMPTY_DIVE_STRING;
}

// combine the pointer address with the trip title so that
// we detect multiple, destinct trips with the same title
// the trip title is designed to be
// location (# dives)
// or, if there is no location name
// date range (# dives)
// where the date range is given as "month year" or "month-month year" or "month year - month year"
QString DiveObjectHelper::tripMeta() const
{
	QString ret = EMPTY_DIVE_STRING;
	struct dive_trip *dt = m_dive->divetrip;
	if (dt) {
		QString numDives = tr("%1 dive(s)").arg(dt->nrdives);
		QString title(dt->location);
		if (title.isEmpty()) {
			// so use the date range
			QDateTime firstTime = QDateTime::fromMSecsSinceEpoch(1000*dt->when, Qt::UTC);
			QString firstMonth = firstTime.toString("MMM");
			QString firstYear = firstTime.toString("yyyy");
			QDateTime lastTime = QDateTime::fromMSecsSinceEpoch(1000*dt->dives->when, Qt::UTC);
			QString lastMonth = lastTime.toString("MMM");
			QString lastYear = lastTime.toString("yyyy");
			if (lastMonth == firstMonth && lastYear == firstYear)
				title = firstMonth + " " + firstYear;
			else if (lastMonth != firstMonth && lastYear == firstYear)
				title = firstMonth + "-" + lastMonth + " " + firstYear;
			else
				title = firstMonth + " " + firstYear + " - " + lastMonth + " " + lastYear;
		}
		ret = QString::number((quint64)m_dive->divetrip, 16) + QLatin1Literal("::") + QStringLiteral("%1 (%2)").arg(title, numDives);
	}
	return ret;
}

QString DiveObjectHelper::maxcns() const
{
	return QString(m_dive->maxcns);
}

QString DiveObjectHelper::otu() const
{
	return QString(m_dive->otu);
}

int DiveObjectHelper::rating() const
{
	return m_dive->rating;
}

QString DiveObjectHelper::sumWeight() const
{
	weight_t sum = { 0 };
	for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++){
		sum.grams += m_dive->weightsystem[i].weight.grams;
	}
	return get_weight_string(sum, true);
}

QString DiveObjectHelper::getCylinder() const
{
	QString getCylinder;
	if (is_cylinder_used(m_dive, 1)){
		getCylinder = QObject::tr("Multiple");
	}
	else {
		getCylinder = m_dive->cylinder[0].type.description;
	}
	return getCylinder;
}

QString DiveObjectHelper::startPressure() const
{
	QString startPressure = getPressures(m_dive, START_PRESSURE);
	return startPressure;
}

QString DiveObjectHelper::endPressure() const
{
	QString endPressure = getPressures(m_dive, END_PRESSURE);
	return endPressure;
}

QString DiveObjectHelper::firstGas() const
{
	QString gas;
	gas = get_gas_string(m_dive->cylinder[0].gasmix);
	return gas;
}

QStringList DiveObjectHelper::suitList() const
{
	QStringList suits;
	struct dive *d;
	int i = 0;
	for_each_dive (i, d) {
		QString temp = d->suit;
		if (!temp.isEmpty())
			suits << d->suit;
	}
       suits.removeDuplicates();
       suits.sort();
       return suits;
}

QStringList DiveObjectHelper::buddyList() const
{
	QStringList buddies;
	struct dive *d;
	int i = 0;
	for_each_dive (i, d) {
		QString temp = d->buddy;
		if (!temp.isEmpty() && !temp.contains(",")){
			buddies << d->buddy;
		}
		else if (!temp.isEmpty()){
			QRegExp sep("(,\\s)");
			QStringList tempList = temp.split(sep);
			buddies << tempList;
		}
	}
       buddies.removeDuplicates();
       buddies.sort();
       return buddies;
}

QStringList DiveObjectHelper::divemasterList() const
{
	QStringList divemasters;
	struct dive *d;
	int i = 0;
	for_each_dive (i, d) {
		QString temp = d->divemaster;
		if (!temp.isEmpty())
			divemasters << d->divemaster;
	}
       divemasters.removeDuplicates();
       divemasters.sort();
       return divemasters;
}
