// SPDX-License-Identifier: GPL-2.0
#include "commands.h"
#include "dive_ref.h"
#include "json_serializer.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/file.h"
#include "core/trip.h"
#include "core/qthelper.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// Output JSON to stdout
static void outputJson(const QJsonObject &obj)
{
	QJsonDocument doc(obj);
	printf("%s\n", doc.toJson(QJsonDocument::Indented).constData());
}

// Create an error response
static QJsonObject errorResponse(const QString &code, const QString &message)
{
	QJsonObject obj;
	obj["status"] = "error";
	obj["error_code"] = code;
	obj["message"] = message;
	return obj;
}

int cmdListDives(const CliConfig &config, int start, int count)
{
	// Validate parameters
	if (start < 0) start = 0;
	if (count <= 0) count = 50;
	if (count > 500) count = 500;

	// Load dive data
	if (parse_file(config.repo_path.toStdString().c_str(), &divelog) < 0) {
		outputJson(errorResponse("REPO_NOT_FOUND",
			QString("Failed to load dive data from: %1").arg(config.repo_path)));
		return CMD_ERROR_IO;
	}

	// Generate and output JSON
	QJsonObject result = diveListToJson(&divelog, start, count);
	outputJson(result);

	return CMD_SUCCESS;
}

int cmdGetDive(const CliConfig &config, const QString &diveRef)
{
	if (diveRef.isEmpty()) {
		outputJson(errorResponse("INVALID_ARGS", "dive-ref parameter is required"));
		return CMD_ERROR_INVALID_ARGS;
	}

	// Load dive data
	if (parse_file(config.repo_path.toStdString().c_str(), &divelog) < 0) {
		outputJson(errorResponse("REPO_NOT_FOUND",
			QString("Failed to load dive data from: %1").arg(config.repo_path)));
		return CMD_ERROR_IO;
	}

	// Find the dive
	struct dive *d = findDiveByRef(diveRef);
	if (!d) {
		outputJson(errorResponse("NOT_FOUND",
			QString("Dive not found: %1").arg(diveRef)));
		return CMD_ERROR_NOT_FOUND;
	}
	// let's get all the indirect data populated
	d->fixup_dive();

	// Generate response
	QJsonObject result;
	result["status"] = "success";
	result["dive"] = diveToJsonFull(d);
	outputJson(result);

	return CMD_SUCCESS;
}

int cmdGetTrip(const CliConfig &config, const QString &tripRef)
{
	if (tripRef.isEmpty()) {
		outputJson(errorResponse("INVALID_ARGS", "trip-ref parameter is required"));
		return CMD_ERROR_INVALID_ARGS;
	}

	// Load dive data
	if (parse_file(config.repo_path.toStdString().c_str(), &divelog) < 0) {
		outputJson(errorResponse("REPO_NOT_FOUND",
			QString("Failed to load dive data from: %1").arg(config.repo_path)));
		return CMD_ERROR_IO;
	}

	// Find the trip
	struct dive_trip *trip = findTripByRef(tripRef);
	if (!trip) {
		outputJson(errorResponse("NOT_FOUND",
			QString("Trip not found: %1").arg(tripRef)));
		return CMD_ERROR_NOT_FOUND;
	}

	// Generate response with trip info and dives
	QJsonObject result;
	result["status"] = "success";
	result["trip"] = tripToJson(trip);

	// Add dives in the trip
	QJsonArray dives;
	for (const auto *d : trip->dives) {
		dives.append(diveToJsonSummary(d));
	}
	result["dives"] = dives;

	outputJson(result);
	return CMD_SUCCESS;
}

int cmdGetProfile(const CliConfig &config, const QString &diveRef, int dcIndex, int width, int height)
{
	Q_UNUSED(config);
	Q_UNUSED(diveRef);
	Q_UNUSED(dcIndex);
	Q_UNUSED(width);
	Q_UNUSED(height);

	// Profile rendering is not yet implemented in the CLI tool.
	// This requires refactoring the profile-widget library to separate
	// the rendering code from the interactive desktop widget code.
	outputJson(errorResponse("NOT_IMPLEMENTED",
		"Profile rendering is not yet available in the CLI tool"));
	return CMD_ERROR_INTERNAL;
}

int cmdGetStats(const CliConfig &config)
{
	// Load dive data
	if (parse_file(config.repo_path.toStdString().c_str(), &divelog) < 0) {
		outputJson(errorResponse("REPO_NOT_FOUND",
			QString("Failed to load dive data from: %1").arg(config.repo_path)));
		return CMD_ERROR_IO;
	}

	// Build response
	QJsonObject result;
	result["status"] = "success";
	result["total_dives"] = static_cast<int>(divelog.dives.size());
	result["total_trips"] = static_cast<int>(divelog.trips.size());
	result["statistics"] = statsToJson(&divelog);

	outputJson(result);
	return CMD_SUCCESS;
}
