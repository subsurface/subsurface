#include "string-format.h"
#include "deco.h"
#include "dive.h"
#include "divesite.h"
#include "membuffer.h"
#include "profile.h"
#include "qthelper.h"
#include "subsurface-string.h"
#include "trip.h"
#include <QDateTime>
#include <QLocale>
#include <QTextDocument>

enum returnPressureSelector { START_PRESSURE, END_PRESSURE };
static QLocale loc;

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

QString formatTripTitle(const dive_trip *trip)
{
	if (!trip)
		return QString();

	timestamp_t when = trip_date(trip);
	bool getday = trip_is_single_day(trip);

	QDateTime localTime = timestampToDateTime(when);

	QString prefix = !empty_string(trip->location) ? QString(trip->location) + ", " : QString();
	if (getday)
		return prefix + loc.toString(localTime, prefs.date_format);
	else
		return prefix + loc.toString(localTime, "MMM yyyy");
}

QString formatTripTitleWithDives(const dive_trip *trip)
{
	int nr = trip->dives.nr;
	return formatTripTitle(trip) + " " +
	       gettextFromC::tr("(%n dive(s))", "", nr);
}

#define DIV_UP(x, y) (((x) + (y) - 1) / (y))
#define translate(x,y) qPrintable(QCoreApplication::translate(x,y))

static QString formatPlotInfoInternal(const dive *d, const plot_info *pi, int idx)
{
	membufferpp b;
	int pressurevalue, mod, ead, end, eadd;
	const char *depth_unit, *pressure_unit, *temp_unit, *vertical_speed_unit;
	double depthvalue, tempvalue, speedvalue, sacvalue;
	int decimals, cyl;
	const char *unit;
	const struct plot_data *entry = pi->entry + idx;

	depthvalue = get_depth_units(entry->depth, NULL, &depth_unit);
	put_format_loc(&b, "%s: %d:%02d\n", translate("gettextFromC", "@"), FRACTION(entry->sec, 60));
	put_format_loc(&b, "%s: %.1f%s\n", translate("gettextFromC", "D"), depthvalue, depth_unit);
	for (cyl = 0; cyl < pi->nr_cylinders; cyl++) {
		int mbar = get_plot_pressure(pi, idx, cyl);
		if (!mbar)
			continue;
		struct gasmix mix = get_cylinder(d, cyl)->gasmix;
		pressurevalue = get_pressure_units(mbar, &pressure_unit);
		put_format_loc(&b, "%s: %d%s (%s)\n", translate("gettextFromC", "P"), pressurevalue, pressure_unit, gasname(mix));
	}
	if (entry->temperature) {
		tempvalue = get_temp_units(entry->temperature, &temp_unit);
		put_format_loc(&b, "%s: %.1f%s\n", translate("gettextFromC", "T"), tempvalue, temp_unit);
	}
	speedvalue = get_vertical_speed_units(abs(entry->speed), NULL, &vertical_speed_unit);
	/* Ascending speeds are positive, descending are negative */
	if (entry->speed > 0)
		speedvalue *= -1;
	put_format_loc(&b, "%s: %.1f%s\n", translate("gettextFromC", "V"), speedvalue, vertical_speed_unit);
	sacvalue = get_volume_units(entry->sac, &decimals, &unit);
	if (entry->sac && prefs.show_sac)
		put_format_loc(&b, "%s: %.*f%s/%s\n", translate("gettextFromC", "SAC"), decimals, sacvalue, unit, translate("gettextFromC", "min"));
	if (entry->cns)
		put_format_loc(&b, "%s: %u%%\n", translate("gettextFromC", "CNS"), entry->cns);
	if (prefs.pp_graphs.po2 && entry->pressures.o2 > 0) {
		put_format_loc(&b, "%s: %.2f%s\n", translate("gettextFromC", "pO₂"), entry->pressures.o2, translate("gettextFromC", "bar"));
		if (entry->scr_OC_pO2.mbar)
			put_format_loc(&b, "%s: %.2f%s\n", translate("gettextFromC", "SCR ΔpO₂"), entry->scr_OC_pO2.mbar/1000.0 - entry->pressures.o2,
					translate("gettextFromC", "bar"));
	}
	if (prefs.pp_graphs.pn2 && entry->pressures.n2 > 0)
		put_format_loc(&b, "%s: %.2f%s\n", translate("gettextFromC", "pN₂"), entry->pressures.n2, translate("gettextFromC", "bar"));
	if (prefs.pp_graphs.phe && entry->pressures.he > 0)
		put_format_loc(&b,"%s: %.2f%s\n", translate("gettextFromC", "pHe"), entry->pressures.he, translate("gettextFromC", "bar"));
	if (prefs.mod && entry->mod > 0) {
		mod = lrint(get_depth_units(entry->mod, NULL, &depth_unit));
		put_format_loc(&b, "%s: %d%s\n", translate("gettextFromC", "MOD"), mod, depth_unit);
	}
	eadd = lrint(get_depth_units(entry->eadd, NULL, &depth_unit));

	if (prefs.ead) {
		switch (pi->dive_type) {
		case plot_info::NITROX:
			if (entry->ead > 0) {
				ead = lrint(get_depth_units(entry->ead, NULL, &depth_unit));
				put_format_loc(&b, "%s: %d%s\n", translate("gettextFromC", "EAD"), ead, depth_unit);
				put_format_loc(&b, "%s: %d%s / %.1fg/ℓ\n", translate("gettextFromC", "EADO"), eadd, depth_unit, entry->density);
				break;
			}
		case plot_info::TRIMIX:
			if (entry->end > 0) {
				end = lrint(get_depth_units(entry->end, NULL, &depth_unit));
				put_format_loc(&b, "%s: %d%s\n", translate("gettextFromC", "END"), end, depth_unit);
				put_format_loc(&b, "%s: %d%s / %.1fg/ℓ\n", translate("gettextFromC", "EADO"), eadd, depth_unit, entry->density);
				break;
			}
		case plot_info::AIR:
			if (entry->density > 0) {
				put_format_loc(&b, "%s: %.1fg/ℓ\n", translate("gettextFromC", "Density"), entry->density);
			}
		case plot_info::FREEDIVING:
			/* nothing */
			break;
		}
	}
	if (entry->stopdepth) {
		depthvalue = get_depth_units(entry->stopdepth, NULL, &depth_unit);
		if (entry->ndl > 0) {
			/* this is a safety stop as we still have ndl */
			if (entry->stoptime)
				put_format_loc(&b, "%s: %u%s @ %.0f%s\n", translate("gettextFromC", "Safety stop"), DIV_UP(entry->stoptime, 60),
					       translate("gettextFromC", "min"), depthvalue, depth_unit);
			else
				put_format_loc(&b, "%s: %s @ %.0f%s\n", translate("gettextFromC", "Safety stop"),
					       translate("gettextFromC", "unknown time"), depthvalue, depth_unit);
		} else {
			/* actual deco stop */
			if (entry->stoptime)
				put_format_loc(&b, "%s: %u%s @ %.0f%s\n", translate("gettextFromC", "Deco"), DIV_UP(entry->stoptime, 60),
					       translate("gettextFromC", "min"), depthvalue, depth_unit);
			else
				put_format_loc(&b, "%s: %s @ %.0f%s\n", translate("gettextFromC", "Deco"),
					       translate("gettextFromC", "unknown time"), depthvalue, depth_unit);
		}
	} else if (entry->in_deco) {
		put_string(&b, translate("gettextFromC", "In deco\n"));
	} else if (entry->ndl >= 0) {
		put_format_loc(&b, "%s: %u%s\n", translate("gettextFromC", "NDL"), DIV_UP(entry->ndl, 60), translate("gettextFromC", "min"));
	}
	if (entry->tts)
		put_format_loc(&b, "%s: %u%s\n", translate("gettextFromC", "TTS"), DIV_UP(entry->tts, 60), translate("gettextFromC", "min"));
	if (entry->stopdepth_calc && entry->stoptime_calc) {
		depthvalue = get_depth_units(entry->stopdepth_calc, NULL, &depth_unit);
		put_format_loc(&b, "%s: %u%s @ %.0f%s (%s)\n", translate("gettextFromC", "Deco"), DIV_UP(entry->stoptime_calc, 60),
			       translate("gettextFromC", "min"), depthvalue, depth_unit, translate("gettextFromC", "calc"));
	} else if (entry->in_deco_calc) {
		/* This means that we have no NDL left,
		 * and we have no deco stop,
		 * so if we just accend to the surface slowly
		 * (ascent_mm_per_step / ascent_s_per_step)
		 * everything will be ok. */
		put_string(&b, translate("gettextFromC", "In deco (calc)\n"));
	} else if (prefs.calcndltts && entry->ndl_calc != 0) {
		if(entry->ndl_calc < MAX_PROFILE_DECO)
			put_format_loc(&b, "%s: %u%s (%s)\n", translate("gettextFromC", "NDL"), DIV_UP(entry->ndl_calc, 60),
				       translate("gettextFromC", "min"), translate("gettextFromC", "calc"));
		else
			put_string(&b, translate("gettextFromC", "NDL: >2h (calc)\n"));
	}
	if (entry->tts_calc) {
		if (entry->tts_calc < MAX_PROFILE_DECO)
			put_format_loc(&b, "%s: %u%s (%s)\n", translate("gettextFromC", "TTS"), DIV_UP(entry->tts_calc, 60),
				       translate("gettextFromC", "min"), translate("gettextFromC", "calc"));
		else
			put_string(&b, translate("gettextFromC", "TTS: >2h (calc)\n"));
	}
	if (entry->rbt)
		put_format_loc(&b, "%s: %u%s\n", translate("gettextFromC", "RBT"), DIV_UP(entry->rbt, 60), translate("gettextFromC", "min"));
	if (prefs.decoinfo) {
		if (entry->current_gf > 0.0)
			put_format(&b, "%s %d%%\n", translate("gettextFromC", "GF"), (int)(100.0 * entry->current_gf));
		if (entry->surface_gf > 0.0)
			put_format(&b, "%s %.0f%%\n", translate("gettextFromC", "Surface GF"), entry->surface_gf);
		if (entry->ceiling) {
			depthvalue = get_depth_units(entry->ceiling, NULL, &depth_unit);
			put_format_loc(&b, "%s %.1f%s\n", translate("gettextFromC", "Calculated ceiling"), depthvalue, depth_unit);
			if (prefs.calcalltissues) {
				int k;
				for (k = 0; k < 16; k++) {
					if (entry->ceilings[k]) {
						depthvalue = get_depth_units(entry->ceilings[k], NULL, &depth_unit);
						put_format_loc(&b, "%s %.0f%s: %.1f%s\n", translate("gettextFromC", "Tissue"),
								   buehlmann_N2_t_halflife[k], translate("gettextFromC", "min"),
								   depthvalue, depth_unit);
					}
				}
			}
		}
	}
	if (entry->icd_warning)
		put_string(&b, translate("gettextFromC", "ICD in leading tissue\n"));
	if (entry->heartbeat && prefs.hrgraph)
		put_format_loc(&b, "%s: %d\n", translate("gettextFromC", "heart rate"), entry->heartbeat);
	if (entry->bearing >= 0)
		put_format_loc(&b, "%s: %d\n", translate("gettextFromC", "bearing"), entry->bearing);
	if (entry->running_sum) {
		depthvalue = get_depth_units(entry->running_sum / entry->sec, NULL, &depth_unit);
		put_format_loc(&b, "%s %.1f%s\n", translate("gettextFromC", "mean depth to here"), depthvalue, depth_unit);
	}

	strip_mb(&b);
	return QString(mb_cstring(&b));
}

std::pair<QString, int> formatProfileInfo(const struct dive *d, const struct plot_info *pi, int time)
{
	int i;

	/* The two first and the two last plot entries do not have useful data */
	if (pi->nr <= 4)
		return std::make_pair(QString(), 0);
	for (i = 2; i < pi->nr - 3; i++) {
		if (pi->entry[i].sec >= time)
			break;
	}
	QString res = formatPlotInfoInternal(d, pi, i);
	return std::make_pair(res, i);
}
