// SPDX-License-Identifier: GPL-2.0
#include "json_serializer.h"
#include "dive_ref.h"
#include "core/dive.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/divelog.h"
#include "core/sample.h"
#include "core/tag.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include <QJsonArray>
#include <cmath>
#include <limits>

// Helper to format date as ISO 8601 date string
static QString formatDate(timestamp_t when)
{
	struct tm tm;
	utc_mkdate(when, &tm);
	return QString("%1-%2-%3")
		.arg(tm.tm_year, 4, 10, QChar('0'))
		.arg(tm.tm_mon + 1, 2, 10, QChar('0'))
		.arg(tm.tm_mday, 2, 10, QChar('0'));
}

// Helper to format time as HH:MM:SS
static QString formatTime(timestamp_t when)
{
	struct tm tm;
	utc_mkdate(when, &tm);
	return QString("%1:%2:%3")
		.arg(tm.tm_hour, 2, 10, QChar('0'))
		.arg(tm.tm_min, 2, 10, QChar('0'))
		.arg(tm.tm_sec, 2, 10, QChar('0'));
}

// Convert depth from mm to meters
static double depthToMeters(int mm)
{
	return mm / 1000.0;
}

// Convert duration from seconds to minutes
static int durationToMinutes(int seconds)
{
	return (seconds + 30) / 60; // round to nearest minute
}

// Convert temperature from milli-Kelvin to Celsius
static double tempToCelsius(temperature_t temp)
{
	if (temp.mkelvin == 0)
		return 0.0;
	return (temp.mkelvin - ZERO_C_IN_MKELVIN) / 1000.0;
}

// Convert pressure from mbar to bar
static double pressureToBar(int mbar)
{
	return mbar / 1000.0;
}

// Get gas name from gasmix
static QString gasName(struct gasmix mix)
{
	if (gasmix_is_air(mix))
		return "Air";
	if (mix.he.permille == 0)
		return QString("EAN%1").arg(get_o2(mix) / 10);
	return QString("Tx%1/%2").arg(get_o2(mix) / 10).arg(get_he(mix) / 10);
}

QJsonObject diveToJsonSummary(const struct dive *d)
{
	QJsonObject obj;

	obj["type"] = "dive";
	obj["dive_ref"] = getDiveRef(d);
	obj["dive_number"] = d->number;
	obj["date"] = formatDate(d->when);
	obj["time"] = formatTime(d->when);
	obj["location"] = QString::fromStdString(d->get_location());
	obj["duration_minutes"] = durationToMinutes(d->duration.seconds);
	obj["max_depth_meters"] = depthToMeters(d->maxdepth.mm);

	if (!d->buddy.empty())
		obj["buddy"] = QString::fromStdString(d->buddy);

	// Build list of unique gases used
	QJsonArray gases;
	for (const auto &cyl : d->cylinders) {
		QString name = gasName(cyl.gasmix);
		// Only add if not already present
		bool found = false;
		for (int i = 0; i < gases.size(); i++) {
			if (gases[i].toString() == name) {
				found = true;
				break;
			}
		}
		if (!found)
			gases.append(name);
	}
	obj["gases"] = gases;

	if (d->sac > 0)
		obj["sac_rate"] = d->sac / 1000.0; // ml/min to l/min

	obj["dc_count"] = static_cast<int>(d->dcs.size());

	return obj;
}

QJsonObject diveToJsonFull(const struct dive *d)
{
	QJsonObject obj;

	obj["dive_ref"] = getDiveRef(d);
	obj["dive_number"] = d->number;
	obj["date"] = formatDate(d->when);
	obj["time"] = formatTime(d->when);
	obj["duration_minutes"] = durationToMinutes(d->duration.seconds);
	obj["max_depth_meters"] = depthToMeters(d->maxdepth.mm);
	obj["avg_depth_meters"] = depthToMeters(d->meandepth.mm);

	// Trip reference if in a trip
	if (d->divetrip)
		obj["trip_ref"] = getTripRef(d->divetrip);

	// Location information
	QJsonObject location;
	location["name"] = QString::fromStdString(d->get_location());
	if (d->dive_site && d->dive_site->has_gps_location()) {
		location["gps"] = QString("%1,%2")
			.arg(d->dive_site->location.lat.udeg / 1000000.0, 0, 'f', 6)
			.arg(d->dive_site->location.lon.udeg / 1000000.0, 0, 'f', 6);
	}
	obj["location"] = location;

	// People
	if (!d->buddy.empty())
		obj["buddy"] = QString::fromStdString(d->buddy);
	if (!d->diveguide.empty())
		obj["diveguide"] = QString::fromStdString(d->diveguide);
	if (!d->suit.empty())
		obj["suit"] = QString::fromStdString(d->suit);

	// Tags
	QJsonArray tags;
	for (const auto &tag : d->tags)
		tags.append(QString::fromStdString(tag->name));
	if (!tags.isEmpty())
		obj["tags"] = tags;

	// Ratings
	if (d->rating > 0)
		obj["rating"] = d->rating;
	if (d->visibility > 0)
		obj["visibility"] = d->visibility;

	// Temperatures
	if (d->airtemp.mkelvin > 0)
		obj["air_temp_celsius"] = tempToCelsius(d->airtemp);
	if (d->watertemp.mkelvin > 0)
		obj["water_temp_celsius"] = tempToCelsius(d->watertemp);

	// Gases
	QJsonArray gases;
	for (const auto &cyl : d->cylinders) {
		if (cyl.type.description.empty())
			continue;
		QJsonObject gas;
		gas["name"] = gasName(cyl.gasmix);
		gas["o2_percent"] = get_o2(cyl.gasmix) / 10;
		gas["he_percent"] = get_he(cyl.gasmix) / 10;
		// Check for duplicates
		bool found = false;
		for (int i = 0; i < gases.size(); i++) {
			if (gases[i].toObject()["name"] == gas["name"]) {
				found = true;
				break;
			}
		}
		if (!found)
			gases.append(gas);
	}
	obj["gases"] = gases;

	// Cylinders
	QJsonArray cylinders;
	for (const auto &cyl : d->cylinders) {
		if (cyl.type.description.empty())
			continue;
		QJsonObject cylObj;
		cylObj["description"] = QString::fromStdString(cyl.type.description);
		if (cyl.start.mbar > 0)
			cylObj["start_pressure_bar"] = pressureToBar(cyl.start.mbar);
		else if (cyl.sample_start.mbar > 0)
			cylObj["start_pressure_bar"] = pressureToBar(cyl.sample_start.mbar);
		if (cyl.end.mbar > 0)
			cylObj["end_pressure_bar"] = pressureToBar(cyl.end.mbar);
		else if (cyl.sample_end.mbar > 0)
			cylObj["end_pressure_bar"] = pressureToBar(cyl.sample_end.mbar);
		cylObj["gas_name"] = gasName(cyl.gasmix);
		cylinders.append(cylObj);
	}
	obj["cylinders"] = cylinders;

	// Weights
	QJsonArray weights;
	for (const auto &ws : d->weightsystems) {
		if (ws.weight.grams == 0)
			continue;
		QJsonObject wsObj;
		wsObj["description"] = QString::fromStdString(ws.description);
		wsObj["weight_kg"] = ws.weight.grams / 1000.0;
		weights.append(wsObj);
	}
	if (!weights.isEmpty())
		obj["weights"] = weights;

	// SAC rate
	if (d->sac > 0)
		obj["sac_rate"] = d->sac / 1000.0;

	// Notes
	if (!d->notes.empty())
		obj["notes"] = QString::fromStdString(d->notes);

	// Dive computers
	QJsonArray dcs;
	int dcIndex = 0;
	for (const auto &dc : d->dcs) {
		QJsonObject dcObj;
		dcObj["dc_index"] = dcIndex++;
		dcObj["model"] = QString::fromStdString(dc.model);
		if (!dc.serial.empty())
			dcObj["device_id"] = QString::fromStdString(dc.serial);
		dcObj["sample_count"] = static_cast<int>(dc.samples.size());
		dcs.append(dcObj);
	}
	obj["dive_computers"] = dcs;

	return obj;
}

QJsonObject tripToJson(const struct dive_trip *trip)
{
	QJsonObject obj;

	obj["trip_ref"] = getTripRef(trip);
	obj["description"] = QString::fromStdString(trip->location);
	obj["location"] = QString::fromStdString(trip->location);
	obj["dive_count"] = static_cast<int>(trip->dives.size());

	// Find date range
	timestamp_t earliest = std::numeric_limits<timestamp_t>::max();
	timestamp_t latest = 0;
	for (const auto *d : trip->dives) {
		if (d->when < earliest)
			earliest = d->when;
		timestamp_t end = d->endtime();
		if (end > latest)
			latest = end;
	}

	if (earliest != std::numeric_limits<timestamp_t>::max()) {
		obj["date_start"] = formatDate(earliest);
		obj["date_end"] = formatDate(latest);
	}

	if (!trip->notes.empty())
		obj["notes"] = QString::fromStdString(trip->notes);

	return obj;
}

QJsonObject statsToJson(const struct divelog *log)
{
	QJsonObject stats;

	int totalTime = 0;
	int maxDepth = 0;
	int totalDepth = 0;
	int diveCount = 0;

	for (const auto &d : log->dives) {
		totalTime += d->duration.seconds;
		if (d->maxdepth.mm > maxDepth)
			maxDepth = d->maxdepth.mm;
		totalDepth += d->meandepth.mm;
		diveCount++;
	}

	stats["total_dive_time_minutes"] = totalTime / 60;
	stats["max_depth_meters"] = depthToMeters(maxDepth);
	if (diveCount > 0)
		stats["avg_depth_meters"] = depthToMeters(totalDepth / diveCount);
	else
		stats["avg_depth_meters"] = 0.0;

	return stats;
}

QJsonObject diveListToJson(const struct divelog *log, int start, int count)
{
	QJsonObject result;
	result["status"] = "success";
	result["total_dives"] = static_cast<int>(log->dives.size());
	result["total_trips"] = static_cast<int>(log->trips.size());
	result["statistics"] = statsToJson(log);

	// Build a combined list of trips and tripless dives
	// Trips are represented once, tripless dives are listed individually
	struct item {
		enum { TRIP, DIVE } type;
		timestamp_t when;
		const dive_trip *trip;
		const dive *d;
	};
	std::vector<item> items;

	// Add all trips
	for (const auto &trip : log->trips) {
		item i;
		i.type = item::TRIP;
		i.when = trip->date();
		i.trip = trip.get();
		i.d = nullptr;
		items.push_back(i);
	}

	// Add tripless dives
	for (const auto &d : log->dives) {
		if (!d->divetrip) {
			item i;
			i.type = item::DIVE;
			i.when = d->when;
			i.trip = nullptr;
			i.d = d.get();
			items.push_back(i);
		}
	}

	// Sort by date (newest first)
	std::sort(items.begin(), items.end(), [](const item &a, const item &b) {
		return a.when > b.when;
	});

	// Apply pagination
	QJsonArray itemsArray;
	int end = std::min(start + count, static_cast<int>(items.size()));
	for (int i = start; i < end; i++) {
		const auto &it = items[i];
		if (it.type == item::TRIP) {
			QJsonObject tripObj = tripToJson(it.trip);
			tripObj["type"] = "trip";
			itemsArray.append(tripObj);
		} else {
			itemsArray.append(diveToJsonSummary(it.d));
		}
	}

	result["items"] = itemsArray;
	return result;
}
