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
	m_maxcns(d->maxcns),
	m_otu(d->otu),
	m_dive(d)
{
	struct dive_site *ds = get_dive_site_by_uuid(d->dive_site_uuid);
	if (ds)
		m_gps = QString("%1,%2").arg(ds->latitude.udeg / 1000000.0).arg(ds->longitude.udeg / 1000000.0);

	m_notes = QString::fromUtf8(d->notes);
	if (m_notes.isEmpty())
		m_notes = EMPTY_DIVE_STRING;
	if (same_string(d->dc.model, "planned dive")) {
		QTextDocument notes;
		QString notesFormatted = m_notes;
#define _NOTES_BR "&#92n"
		notesFormatted = notesFormatted.replace("<thead>", "<thead>" _NOTES_BR);
		notesFormatted = notesFormatted.replace("<br>", "<br>" _NOTES_BR);
		notesFormatted = notesFormatted.replace("<tr>", "<tr>" _NOTES_BR);
		notesFormatted = notesFormatted.replace("</tr>", "</tr>" _NOTES_BR);
		notes.setHtml(notesFormatted);
		m_notes = notes.toPlainText();
		m_notes.replace(_NOTES_BR, "<br>");
#undef _NOTES_BR
	} else {
		m_notes.replace("\n", "<br>");
	}


	char buffer[256];
	taglist_get_tagstring(d->tag_list, buffer, 256);
	m_tags = QString(buffer);


	int added = 0;
	QString gas, gases;
	for (int i = 0; i < MAX_CYLINDERS; i++) {
		if (!is_cylinder_used(d, i))
			continue;
		gas = d->cylinder[i].type.description;
		gas += QString(!gas.isEmpty() ? " " : "") + gasname(&d->cylinder[i].gasmix);
		// if has a description and if such gas is not already present
		if (!gas.isEmpty() && gases.indexOf(gas) == -1) {
			if (added > 0)
				gases += QString(" / ");
			gases += gas;
			added++;
		}
	}
	m_gas = gases;

	if (d->sac) {
		const char *unit;
		int decimal;
		double value = get_volume_units(d->sac, &decimal, &unit);
		m_sac = QString::number(value, 'f', decimal).append(unit);
	}

	for (int i = 0; i < MAX_CYLINDERS; i++)
		m_cylinders << getFormattedCylinder(d, i);

	for (int i = 0; i < MAX_WEIGHTSYSTEMS; i++)
		m_weights << getFormattedWeight(d, i);

	QDateTime localTime = QDateTime::fromTime_t(d->when - gettimezoneoffset(d->when));
	localTime.setTimeSpec(Qt::UTC);
	m_date = localTime.date().toString(prefs.date_format);
	m_time = localTime.time().toString(prefs.time_format);
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
	return m_date;
}

timestamp_t DiveObjectHelper::timestamp() const
{
	return m_dive->when;
}

QString DiveObjectHelper::time() const
{
	return m_time;
}

QString DiveObjectHelper::location() const
{
	return get_dive_location(m_dive) ? QString::fromUtf8(get_dive_location(m_dive)) : EMPTY_DIVE_STRING;
}

QString DiveObjectHelper::gps() const
{
	return m_gps;
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
	return m_notes;
}

QString DiveObjectHelper::tags() const
{
	return m_tags;
}

QString DiveObjectHelper::gas() const
{
	return m_gas;
}

QString DiveObjectHelper::sac() const
{
	return m_sac;
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
	return m_maxcns;
}

QString DiveObjectHelper::otu() const
{
	return m_otu;
}

int DiveObjectHelper::rating() const
{
	return m_dive->rating;
}
