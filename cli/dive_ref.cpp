// SPDX-License-Identifier: GPL-2.0
#include "dive_ref.h"
#include "core/dive.h"
#include "core/trip.h"
#include "core/divelog.h"
#include "core/qthelper.h"
#include <QRegularExpression>

// Weekday names matching save-git.cpp
static const char *weekdays[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

QString getDiveRef(const struct dive *d)
{
	if (!d || d->when == 0)
		return QString();

	struct tm tm;
	utc_mkdate(d->when, &tm);

	// Format: yyyy/mm/dd-DDD-hh=mm=ss
	return QString("%1/%2/%3-%4-%5=%6=%7")
		.arg(tm.tm_year, 4, 10, QChar('0'))
		.arg(tm.tm_mon + 1, 2, 10, QChar('0'))
		.arg(tm.tm_mday, 2, 10, QChar('0'))
		.arg(weekdays[tm.tm_wday])
		.arg(tm.tm_hour, 2, 10, QChar('0'))
		.arg(tm.tm_min, 2, 10, QChar('0'))
		.arg(tm.tm_sec, 2, 10, QChar('0'));
}

// Convert a freeform string into a URL-safe slug.
// Only keeps [a-zA-Z0-9], replaces spaces/underscores with hyphens,
// drops everything else, collapses consecutive hyphens, and truncates.
static QString slugify(const QString &input, int maxLen = 30)
{
	QString result;
	result.reserve(maxLen);
	bool lastWasHyphen = true; // true to skip leading hyphens

	for (int i = 0; i < input.length() && result.length() < maxLen; i++) {
		QChar c = input[i];
		if (c.isLetterOrNumber() && c.unicode() < 128) {
			result += c;
			lastWasHyphen = false;
		} else if ((c == ' ' || c == '_' || c == '-') && !lastWasHyphen) {
			result += '-';
			lastWasHyphen = true;
		}
		// all other characters are silently dropped
	}

	// Remove trailing hyphen
	while (result.endsWith('-'))
		result.chop(1);

	return result;
}

QString getTripRef(const struct dive_trip *trip)
{
	if (!trip)
		return QString();

	timestamp_t when = trip->date();
	if (when == 0)
		return QString();

	struct tm tm;
	utc_mkdate(when, &tm);

	// Format: yyyy/mm/dd-location-slug
	QString location = slugify(QString::fromStdString(trip->location));
	if (location.isEmpty())
		location = "trip";

	return QString("%1/%2/%3-%4")
		.arg(tm.tm_year, 4, 10, QChar('0'))
		.arg(tm.tm_mon + 1, 2, 10, QChar('0'))
		.arg(tm.tm_mday, 2, 10, QChar('0'))
		.arg(location);
}

struct dive *findDiveByRef(const QString &diveRef)
{
	if (diveRef.isEmpty())
		return nullptr;

	// Validate dive ref length - should be exactly 24 chars (yyyy/mm/dd-DDD-hh=mm=ss)
	// Allow some slack for old colon format, but reject anything too long
	if (diveRef.length() > 30)
		return nullptr;

	// Parse the dive reference: yyyy/mm/dd-DDD-hh=mm=ss
	// Also support old format with colons: yyyy/mm/dd-DDD-hh:mm:ss
	static QRegularExpression re(R"((\d{4})/(\d{2})/(\d{2})-\w{3}-(\d{2})[=:](\d{2})[=:](\d{2}))");
	QRegularExpressionMatch match = re.match(diveRef);

	if (!match.hasMatch())
		return nullptr;

	int year = match.captured(1).toInt();
	int month = match.captured(2).toInt();
	int day = match.captured(3).toInt();
	int hour = match.captured(4).toInt();
	int minute = match.captured(5).toInt();
	int second = match.captured(6).toInt();

	// Convert to timestamp
	struct tm tm = {};
	tm.tm_year = year;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hour;
	tm.tm_min = minute;
	tm.tm_sec = second;
	timestamp_t when = utc_mktime(&tm);

	// Search for dive with this timestamp
	for (auto &d : divelog.dives) {
		if (d->when == when)
			return d.get();
	}

	return nullptr;
}

struct dive_trip *findTripByRef(const QString &tripRef)
{
	if (tripRef.isEmpty())
		return nullptr;

	// Validate trip ref length to prevent DoS with huge strings
	if (tripRef.length() > 100)
		return nullptr;

	// Parse the trip reference: yyyy/mm/dd-location-slug
	// Slug is restricted to [a-zA-Z0-9-] (URL-safe characters only)
	static QRegularExpression re(R"((\d{4})/(\d{2})/(\d{2})-([a-zA-Z0-9](?:[a-zA-Z0-9-]{0,28}[a-zA-Z0-9])?))");
	QRegularExpressionMatch match = re.match(tripRef);

	if (!match.hasMatch())
		return nullptr;

	int year = match.captured(1).toInt();
	int month = match.captured(2).toInt();
	int day = match.captured(3).toInt();
	QString locationSlug = match.captured(4);

	// Convert to approximate timestamp (start of day)
	struct tm tm = {};
	tm.tm_year = year;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;
	timestamp_t dayStart = utc_mktime(&tm);
	timestamp_t dayEnd = dayStart + 24 * 60 * 60;

	// Search for trip that starts on this day and has matching location slug
	for (auto &trip : divelog.trips) {
		timestamp_t tripDate = trip->date();
		if (tripDate >= dayStart && tripDate < dayEnd) {
			QString tripSlug = slugify(QString::fromStdString(trip->location));
			if (tripSlug.isEmpty())
				tripSlug = "trip";
			if (tripSlug == locationSlug)
				return trip.get();
		}
	}

	return nullptr;
}
