// SPDX-License-Identifier: GPL-2.0
#ifndef CLI_JSON_SERIALIZER_H
#define CLI_JSON_SERIALIZER_H

#include <QJsonObject>
#include <QJsonArray>

struct dive;
struct dive_trip;
struct divelog;

// Serialize a dive to JSON (summary for list view)
QJsonObject diveToJsonSummary(const struct dive *d);

// Serialize a dive to JSON (full details)
QJsonObject diveToJsonFull(const struct dive *d);

// Serialize a trip to JSON
QJsonObject tripToJson(const struct dive_trip *trip);

// Serialize dive statistics
QJsonObject statsToJson(const struct divelog *log);

// Serialize a list of dives (with pagination)
QJsonObject diveListToJson(const struct divelog *log, int start, int count);

#endif // CLI_JSON_SERIALIZER_H
