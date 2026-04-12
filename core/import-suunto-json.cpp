// SPDX-License-Identifier: GPL-2.0

/*
 * Import support for JSON dive logs exported from the Suunto app.
 * This aims to support the following models:
 * Suunto Nautic, Nautic S, Ocean, EON Core, EON Steel, D5.
 *
 *
 * Tested configurations:
 *
 *   Model          Air   Nitrox   Trimix   No AI   Multi cyl   Multi gas
 *   -----------    ---   ------   ------   -----   ---------   ---------
 *   Nautic         yes   no       no       yes     yes         no
 *   Nautic S       no    no       no       no      no          no
 *   Ocean          yes   no       no       no      no          no
 *   EON Core       yes   yes      no       no      no          no
 *   EON Steel      no    yes      no       no      no          no
 *   D5             no    no       no       no      no          no
 *
 * Even though cylinder size is configurable in the Nautic and Ocean,
 * it is not available in the JSON
 */

#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "divesite.h"
#include "errorhelper.h"
#include "gettext.h"
#include "sample.h"
#include "subsurface-time.h"

#include <libdivecomputer/parser.h>

/* Suunto app exports all activities (swimming, running, etc.).
 * ActivityType 51 is scuba diving -- skip everything else. */
static const int SUUNTO_ACTIVITY_SCUBA = 51;

/* Parse ISO 8601 timestamp with timezone offset, return ms since epoch (UTC).
 * Returns 0 on parse failure. */
static int64_t parse_iso8601_ms(const std::string &ts)
{
	struct tm tm = {};
	int ms = 0, tz_h = 0, tz_m = 0;
	char tz_sign = '+';
	int n = sscanf(ts.c_str(), "%d-%d-%dT%d:%d:%d.%d%c%d:%d",
		       &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
		       &tm.tm_hour, &tm.tm_min, &tm.tm_sec,
		       &ms, &tz_sign, &tz_h, &tz_m);
	if (n < 9) {
		report_info("Suunto JSON: failed to parse timestamp '%s' (matched %d fields)",
			    ts.c_str(), n);
		return 0;
	}
	tm.tm_year -= 1900;
	tm.tm_mon  -= 1;
	int64_t epoch_s  = utc_mktime(&tm);
	int64_t offset_s = tz_h * 3600LL + tz_m * 60LL;
	return (epoch_s + (tz_sign == '+' ? -offset_s : offset_s)) * 1000LL + ms;
}

// Extract the time from a sample and return milliseconds since epoch.
static int64_t parse_timestamp_ms(const QJsonObject &obj)
{
	std::string ts = obj["TimeISO8601"].toString().toStdString();
	if (ts.empty())
		return 0;
	return parse_iso8601_ms(ts);
}

/* Temporary storage for temperature readings, so they can be matched to
 * samples by timestamp. */
struct temp_reading {
	int64_t timestamp_ms;
	double kelvin;
};

/* Find the closest temperature reading to a given timestamp.
 * Returns the temperature in Kelvin, or 0 if no reading is within 15 seconds. */
static double find_temperature(const std::vector<temp_reading> &readings,
			       int64_t ts_ms)
{
	double best_temp = 0;
	int64_t best_diff = INT64_MAX;
	for (const temp_reading &tr : readings) {
		int64_t diff = tr.timestamp_ms > ts_ms ?
			tr.timestamp_ms - ts_ms :
			ts_ms - tr.timestamp_ms;
		if (diff < best_diff) {
			best_diff = diff;
			best_temp = tr.kelvin;
		}
	}
	if (best_diff < 15000)
		return best_temp;
	return 0;
}

/* Suunto uses internal device names in the JSON, the commercial
 * product names should be shown in Subsurface. */
static std::string map_device_name(const std::string &internal_name)
{
	if (internal_name == "Vaasa")
		return "Suunto Nautic";
	if (internal_name == "Porvoo")
		return "Suunto Ocean";
	/* Fall back to "Suunto <codename>" for unknown devices
	 * This covers the EON Core and EON Steel automatically */
	return "Suunto " + internal_name;
}

/* Header contains dive summary data (date/time, device info, depth,
 * duration, temperature, notes). */
static void parse_header(const QJsonObject &header, struct dive *d)
{
	std::string datetime = header["DateTime"].toString().toStdString();
	if (!datetime.empty()) {
		int64_t ms = parse_iso8601_ms(datetime);
		if (ms != 0)
			d->when = ms / 1000;
	}

	struct divecomputer &dc = d->dcs[0];
	QJsonObject device = header["Device"].toObject();
	std::string device_name = device["Name"].toString().toStdString();
	if (!device_name.empty())
		dc.model = map_device_name(device_name);

	std::string serial = device["SerialNumber"].toString().toStdString();
	if (!serial.empty()) {
		dc.serial = serial;
		dc.deviceid = calculate_string_hash(serial.c_str());
	}

	QJsonObject info = device["Info"].toObject();
	std::string sw = info["SW"].toString().toStdString();
	if (!sw.empty())
		dc.fw_version = sw;

	std::string hw = info["HW"].toString().toStdString();
	if (!hw.empty())
		add_extra_data(&dc, "HW version", hw);

	QJsonObject depth_obj = header["Depth"].toObject();
	double max_depth = depth_obj["Max"].toDouble(0);
	if (max_depth > 0)
		dc.maxdepth.mm = lrint(max_depth * 1000.0);

	/* Nautic uses "DepthAverage", EON uses "Depth.Avg" */
	double avg_depth = header["DepthAverage"].toDouble(0);
	if (avg_depth <= 0)
		avg_depth = depth_obj["Avg"].toDouble(0);
	if (avg_depth > 0)
		dc.meandepth.mm = lrint(avg_depth * 1000.0);

	/* Nautic uses "DiveTime", EON uses "Duration" */
	double divetime = header["DiveTime"].toDouble(0);
	if (divetime <= 0)
		divetime = header["Duration"].toDouble(0);
	if (divetime > 0)
		dc.duration.seconds = lrint(divetime);

	/* Temperature (in Kelvin)
	 * Suunto stores max/min reversed: "Min" is the higher Kelvin value
	 * (= warmer water), "Max" is the lower Kelvin value (= colder). */
	QJsonObject temp_obj = header["Temperature"].toObject();
	double temp_max = temp_obj["Max"].toDouble(0);
	double temp_min = temp_obj["Min"].toDouble(0);
	if (temp_max > 0 && temp_min > 0) {
		double water_temp = std::min(temp_max, temp_min);
		d->watertemp.mkelvin = lrint(water_temp * 1000.0);
		d->maxtemp.mkelvin = lrint(std::max(temp_max, temp_min) * 1000.0);
		d->mintemp.mkelvin = lrint(std::min(temp_max, temp_min) * 1000.0);
	}

	std::string notes = header["Notes"].toString().toStdString();
	if (!notes.empty() && notes != "null")
		d->notes = notes;

	dc.divemode = OC;
}

/* Two-pass sample parsing. First pass collects all temperature readings
 * (for later matching), dive start time (for elapsed time calculation),
 * GPS location and surface pressure. */
static void parse_samples(const QJsonObject &header, const QJsonArray &samples,
			   struct dive *d, struct divelog *log, int gas_offset)
{
	struct divecomputer &dc = d->dcs[0];
	double surface_pressure_pa = 0;
	int64_t dive_start_ms = 0;

	QJsonObject diving = header["Diving"].toObject();
	if (!diving.isEmpty()) {
		double sp = diving["SurfacePressure"].toDouble(0);
		if (sp > 0) {
			surface_pressure_pa = sp;
			/* Suunto Pa, Subsurface mbar */
			dc.surface_pressure.mbar = lrint(sp / 100.0);
		}
	}

	std::vector<temp_reading> temp_readings;

	for (const QJsonValue &val : samples) {
		QJsonObject s = val.toObject();

		/* Collect temperature readings. The "Temperature" field only
		 * appears in raw sensor samples (not the 
		 * "DeviceInternalTemperature" field, that is the device's
		 * internal temperature) */
		if (s.contains("Temperature")) {
			double temp_k = s["Temperature"].toDouble(0);
			if (temp_k > 0) {
				int64_t ts = parse_timestamp_ms(s);
				temp_readings.push_back({ts, temp_k});
			}
		}

		/* Nautic signals dive start via DiveEvents.DiveStatus */
		if (s.contains("DiveEvents")) {
			QJsonObject events = s["DiveEvents"].toObject();
			if (events["DiveStatus"].toBool(false) &&
			    dive_start_ms == 0) {
				dive_start_ms = parse_timestamp_ms(s);
			}
		}

		/* EON signals dive start via Events[].State "Dive Active" */
		if (dive_start_ms == 0 && s.contains("Events")) {
			QJsonArray events = s["Events"].toArray();
			for (const QJsonValue &ev : events) {
				QJsonObject eo = ev.toObject();
				if (eo.contains("State")) {
					QJsonObject state = eo["State"].toObject();
					if (state["Active"].toBool(false) &&
					    state["Type"].toString() == "Dive Active") {
						dive_start_ms = parse_timestamp_ms(s);
						break;
					}
				}
			}
		}

		if (s.contains("DiveRouteOrigin")) {
			QJsonObject origin = s["DiveRouteOrigin"].toObject();
			double lat = origin["Latitude"].toDouble(0);
			double lon = origin["Longitude"].toDouble(0);
			if (lat != 0 && lon != 0) {
				location_t loc = create_location(lat, lon);
				dive_site *ds = log->sites.create(
					dc.model + " " +
					translate("gettextFromC", "dive"),
					loc);
				d->dive_site = ds;
			}
		}

		if (s.contains("SurfacePressure") && surface_pressure_pa == 0) {
			surface_pressure_pa = s["SurfacePressure"].toDouble(0);
			if (surface_pressure_pa > 0)
				/* Suunto Pa, Subsurface mbar */
				dc.surface_pressure.mbar =
					lrint(surface_pressure_pa / 100.0);
		}
	}

	if (dive_start_ms == 0) {
		report_info("Suunto JSON: could not determine dive start time");
		return;
	}

	/* Skip duplicate timestamps -- Subsurface allows one sample per
	 * second. Suunto often logs multiple samples per second, when two
	 * Depth samples coincide the values are close to identical, so
	 * taking the first is safe. */
	int last_elapsed_secs = -1;

	for (const QJsonValue &val : samples) {
		QJsonObject s = val.toObject();

		int64_t sample_ms = parse_timestamp_ms(s);
		if (sample_ms == 0)
			continue;

		int elapsed_secs = static_cast<int>((sample_ms - dive_start_ms) / 1000);

		if (s.contains("Depth") && elapsed_secs >= 0 &&
		    elapsed_secs != last_elapsed_secs) {
			last_elapsed_secs = elapsed_secs;

			struct sample *sam = prepare_sample(&dc);
			sam->time.seconds = elapsed_secs;

			sam->depth.mm = lrint(s["Depth"].toDouble(0) * 1000.0);

			double temp_k = find_temperature(temp_readings, sample_ms);
			if (temp_k > 0)
				sam->temperature.mkelvin = lrint(temp_k * 1000.0);

			double ceiling = s["Ceiling"].toDouble(0);
			if (ceiling > 0) {
				sam->stopdepth.mm = lrint(ceiling * 1000.0);
				sam->in_deco = true;
			}

			if (s.contains("NoDecTime")) {
				int ndt = s["NoDecTime"].toInt(0);
				sam->ndl.seconds = ndt;
			}

			if (s.contains("TimeToSurface"))
				sam->tts.seconds = s["TimeToSurface"].toInt(0);

			/* Cylinder pressures
			 * When multiple tanks share the same gas, Suunto stores
			 * transmitter readings as "Pressure", "Pressure2",
			 * "Pressure3", etc. on the same GasNumber. Iterate
			 * until we hit a missing gas. */
			if (s.contains("Cylinders")) {
				QJsonArray cyls = s["Cylinders"].toArray();
				for (const QJsonValue &cv : cyls) {
					QJsonObject cyl = cv.toObject();
					int gas_num = cyl["GasNumber"].toInt(0)
						- gas_offset;

					int sensor = gas_num;
					for (int t = 0; ; ++t) {
						QString key = (t == 0)
							? QString("Pressure")
							: QString("Pressure%1")
								.arg(t + 1);
						QJsonValue pval = cyl.value(key);
						if (pval.isUndefined())
							break;
						if (pval.isNull()) {
							++sensor;
							continue;
						}
						/* Suunto Pa, Subsurface mbar */
						double pressure_pa =
							pval.toDouble(0);
						if (pressure_pa > 0) {
							d->get_or_create_cylinder(
								sensor);
							int mbar = lrint(
								pressure_pa
								/ 100.0);
							add_sample_pressure(
								sam, sensor,
								mbar);
						}
						++sensor;
					}

					/* Nautic nests Ventilation (m3/s at
					 * surface pressure) inside each
					 * Cylinders entry. Sum across all
					 * gases for total breathing rate. */
					double ventilation =
						cyl["Ventilation"].toDouble(0);
					if (ventilation > 0) {
						d->get_or_create_cylinder(gas_num);
						sam->sac.mliter += lrint(
							ventilation * 60000000.0);
					}
				}
			}

			/* EON puts Ventilation at sample level */
			double top_ventilation = s["Ventilation"].toDouble(0);
			if (top_ventilation > 0)
				sam->sac.mliter += lrint(
					top_ventilation * 60000000.0);

			append_sample(*sam, &dc);
		}

		bool has_dive_events = s.contains("DiveEvents");
		bool has_events_array = s.contains("Events");
		if (!has_dive_events && !has_events_array)
			continue;

		if (elapsed_secs < 0)
			elapsed_secs = 0;

		/* Nautic stores events in a "DiveEvents" object */
		if (has_dive_events) {
			QJsonObject events = s["DiveEvents"].toObject();

			if (events.contains("GasSwitch")) {
				QJsonObject gs = events["GasSwitch"].toObject();
				int gas_num = gs["GasNumber"].toInt(0) - gas_offset;
				d->get_or_create_cylinder(gas_num);
				add_event(&dc, elapsed_secs,
					  SAMPLE_EVENT_GASCHANGE, 0,
					  gas_num,
					  QT_TRANSLATE_NOOP("gettextFromC",
							    "gaschange"));
			}

			if (events.contains("State")) {
				QJsonObject state = events["State"].toObject();
				std::string type = state["Type"].toString().toStdString();
				if (type == "At Safety Stop") {
					add_event(&dc, elapsed_secs, 0, 0, 0,
						  QT_TRANSLATE_NOOP("gettextFromC",
								    "safety stop"));
				}
			}

			if (events.contains("Warning")) {
				QJsonObject warn = events["Warning"].toObject();
				bool active = warn["Active"].toBool(false);
				std::string type = warn["Type"].toString().toStdString();
				if (active && type == "NoDecoTime") {
					add_event(&dc, elapsed_secs, 0, 0, 0,
						  QT_TRANSLATE_NOOP("gettextFromC",
								    "low NDL"));
				}
			}

			if (events.contains("Notify")) {
				QJsonObject notify = events["Notify"].toObject();
				bool active = notify["Active"].toBool(false);
				std::string type = notify["Type"].toString().toStdString();
				if (active && type == "Depth") {
					add_event(&dc, elapsed_secs, 0, 0, 0,
						  QT_TRANSLATE_NOOP("gettextFromC",
								    "depth alarm"));
				}
			}
		}

		/* EON stores events in an "Events" array */
		if (has_events_array) {
			QJsonArray events = s["Events"].toArray();
			for (const QJsonValue &ev : events) {
				QJsonObject eo = ev.toObject();

				if (eo.contains("GasSwitch")) {
					QJsonObject gs = eo["GasSwitch"].toObject();
					int gas_num = gs["GasNumber"].toInt(0) - gas_offset;
					d->get_or_create_cylinder(gas_num);
					add_event(&dc, elapsed_secs,
						  SAMPLE_EVENT_GASCHANGE, 0,
						  gas_num,
						  QT_TRANSLATE_NOOP("gettextFromC",
								    "gaschange"));
				}

				if (eo.contains("Notify")) {
					QJsonObject notify = eo["Notify"].toObject();
					bool active = notify["Active"].toBool(false);
					std::string type = notify["Type"].toString().toStdString();
					if (active && type == "Safety Stop") {
						add_event(&dc, elapsed_secs, 0, 0, 0,
							  QT_TRANSLATE_NOOP("gettextFromC",
									    "safety stop"));
					}
					if (active && type == "Depth") {
						add_event(&dc, elapsed_secs, 0, 0, 0,
							  QT_TRANSLATE_NOOP("gettextFromC",
									    "depth alarm"));
					}
				}

				if (eo.contains("Alarm")) {
					QJsonObject alarm = eo["Alarm"].toObject();
					bool active = alarm["Active"].toBool(false);
					std::string type = alarm["Type"].toString().toStdString();
					if (active && type == "Ascent Speed") {
						add_event(&dc, elapsed_secs, 0, 0, 0,
							  QT_TRANSLATE_NOOP("gettextFromC",
									    "ascent"));
					}
				}

				if (eo.contains("Warning")) {
					QJsonObject warn = eo["Warning"].toObject();
					bool active = warn["Active"].toBool(false);
					std::string type = warn["Type"].toString().toStdString();
					if (active && type == "Mandatory Safety Stop") {
						add_event(&dc, elapsed_secs, 0, 0, 0,
							  QT_TRANSLATE_NOOP("gettextFromC",
									    "safety stop"));
					}
				}
			}
		}
	}
}

/* Suunto puts gas mix, tank size, and pressures all in one "Gases"
 * array. Multiple cylinders can share the same gas. Assign each gas
 * to a Subsurface cylinder by looking at the order of GasSwitch
 * events during the dive. */

static void record_gas(std::vector<int> &gas_switch_order, int gn)
{
	if (gn < 0)
		return;
	for (int existing : gas_switch_order)
		if (existing == gn)
			return;
	gas_switch_order.push_back(gn);
}

static void parse_gases(const QJsonObject &header, const QJsonArray &samples,
			struct dive *d, int gas_offset)
{
	QJsonObject diving = header["Diving"].toObject();
	if (diving.isEmpty())
		return;

	QJsonArray gases = diving["Gases"].toArray();
	if (gases.isEmpty())
		return;

	// Make an ordered list of unique GasNumbers.
	std::vector<int> gas_switch_order;

	for (const QJsonValue &val : samples) {
		QJsonObject s = val.toObject();

		// DiveEvents.GasSwitch (Nautic)
		if (s.contains("DiveEvents")) {
			QJsonObject events = s["DiveEvents"].toObject();
			if (events.contains("GasSwitch"))
				record_gas(gas_switch_order,
					events["GasSwitch"].toObject()
					["GasNumber"].toInt(-1) - gas_offset);
		}

		// Events[].GasSwitch (EON)
		if (s.contains("Events")) {
			QJsonArray events = s["Events"].toArray();
			for (const QJsonValue &ev : events) {
				QJsonObject eo = ev.toObject();
				if (eo.contains("GasSwitch"))
					record_gas(gas_switch_order,
						eo["GasSwitch"].toObject()
						["GasNumber"].toInt(-1)
						- gas_offset);
			}
		}
	}

	for (int i = 0; i < gases.size() && i < static_cast<int>(gas_switch_order.size()); ++i) {
		QJsonObject gas = gases[i].toObject();
		int cyl_idx = gas_switch_order[i];
		cylinder_t *cyl = d->get_or_create_cylinder(cyl_idx);

		/* Suunto fractions (0.0-1.0), Subsurface permille */
		double o2_frac = gas["Oxygen"].toDouble(0);
		double he_frac = gas["Helium"].toDouble(0);
		if (o2_frac > 0)
			cyl->gasmix.o2.permille = lrint(o2_frac * 1000);
		if (he_frac > 0)
			cyl->gasmix.he.permille = lrint(he_frac * 1000);

		/* Suunto m3, Subsurface ml */
		double tank_size_m3 = gas["TankSize"].toDouble(0);
		if (tank_size_m3 > 0)
			cyl->type.size.mliter = lrint(tank_size_m3 * 1000000.0);

		/* Suunto "TankFillPressure" maps to Subsurface working
		 * pressure (Pa -> mbar) */
		double fill_pressure_pa = gas["TankFillPressure"].toDouble(0);
		if (fill_pressure_pa > 0)
			cyl->type.workingpressure.mbar = lrint(fill_pressure_pa / 100.0);

		/* Suunto Pa, Subsurface mbar */
		double start_pa = gas["StartPressure"].toDouble(0);
		if (start_pa > 0)
			cyl->start.mbar = lrint(start_pa / 100.0);

		/* Suunto Pa, Subsurface mbar */
		double end_pa = gas["EndPressure"].toDouble(0);
		if (end_pa > 0)
			cyl->end.mbar = lrint(end_pa / 100.0);
	}
}

// Import a Suunto JSON file into the divelog.
int suunto_json_import(const std::string &buffer, struct divelog *log)
{
	QByteArray raw_data = QByteArray::fromRawData(buffer.data(), buffer.size());
	QJsonDocument doc = QJsonDocument::fromJson(raw_data);

	if (doc.isNull() || !doc.isObject())
		return 0;

	QJsonObject root = doc.object();
	QJsonObject device_log = root["DeviceLog"].toObject();
	if (device_log.isEmpty())
		return 0;

	QJsonObject header = device_log["Header"].toObject();
	QJsonArray samples = device_log["Samples"].toArray();

	if (header.isEmpty()) {
		report_error("Suunto JSON: DeviceLog present but Header is missing");
		return 0;
	}

	if (samples.isEmpty())
		report_info("Suunto JSON: no samples in dive log");

	int activity_type = header["ActivityType"].toInt(0);
	if (activity_type != SUUNTO_ACTIVITY_SCUBA) {
		report_info("Suunto JSON: skipping non-dive activity type %d",
			    activity_type);
		return 0;
	}

	std::unique_ptr<dive> d = std::make_unique<dive>();

	/* Nautic ("Vaasa") and Ocean ("Porvoo") use 0-indexed GasNumber;
	 * EON Core, EON Steel, and D5 use 1-indexed. */
	std::string device_name =
		header["Device"].toObject()["Name"].toString().toStdString();
	int gas_offset = (device_name == "Vaasa" || device_name == "Porvoo")
		? 0 : 1;

	parse_header(header, d.get());
	parse_samples(header, samples, d.get(), log, gas_offset);
	parse_gases(header, samples, d.get(), gas_offset);

	/* Divers need air. Add at least one cylinder exists even when no tank pod
	 * was paired and no GasSwitch event is present in the log. */
	if (d->cylinders.empty())
		d->get_or_create_cylinder(0);

	log->dives.record_dive(std::move(d));
	return 1;
}
