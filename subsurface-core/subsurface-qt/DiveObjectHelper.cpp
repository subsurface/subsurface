#include "DiveObjectHelper.h"

#include <QDateTime>
#include <QTextDocument>

#include "../qthelper.h"
#include "../helpers.h"

static QString EMPTY_DIVE_STRING = QStringLiteral("--");


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
	fmt += ", " + get_volume_string(cyl->type.size, true, 0);
	fmt += ", " + get_pressure_string(cyl->type.workingpressure, true);
	fmt += ", " + get_pressure_string(cyl->start, false) + " - " + get_pressure_string(cyl->end, true);
	fmt += ", " + get_gas_string(cyl->gasmix);
	return fmt;
}

DiveObjectHelper::DiveObjectHelper(struct dive *d) :
	m_dive(d)
{
	for (int i = 0; i < MAX_CYLINDERS; i++)
		m_cylinders << getFormattedCylinder(d, i);

	for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++)
		m_weights << getFormattedWeight(d, i);
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
	QDateTime localTime = QDateTime::fromTime_t(m_dive->when - gettimezoneoffset(m_dive->when));
	localTime.setTimeSpec(Qt::UTC);
	return localTime.date().toString(prefs.date_format);
}

timestamp_t DiveObjectHelper::timestamp() const
{
	return m_dive->when;
}

QString DiveObjectHelper::time() const
{
	QDateTime localTime = QDateTime::fromTime_t(m_dive->when - gettimezoneoffset(m_dive->when));
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
	QString::number(value, 'f', decimal).append(unit);
}

QStringList DiveObjectHelper::weights() const
{
	return m_weights;
}

QString DiveObjectHelper::weight(int idx) const
{
	if (idx < 0 || idx > m_weights.size() - 1)
		return QString(EMPTY_DIVE_STRING);
	return m_weights.at(idx);
}

QString DiveObjectHelper::suit() const
{
	return m_dive->suit ? m_dive->suit : EMPTY_DIVE_STRING;
}

QStringList DiveObjectHelper::cylinders() const
{
	return m_cylinders;
}

QString DiveObjectHelper::cylinder(int idx) const
{
	if (idx < 0 || idx > m_cylinders.size() - 1)
		return QString(EMPTY_DIVE_STRING);
	return m_cylinders.at(idx);
}

QString DiveObjectHelper::trip() const
{
	return m_dive->divetrip ? m_dive->divetrip->location : EMPTY_DIVE_STRING;
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
