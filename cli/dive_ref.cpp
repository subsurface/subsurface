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

QString getTripRef(const struct dive_trip *trip)
{
	if (!trip)
		return QString();

	timestamp_t when = trip->date();
	if (when == 0)
		return QString();

	struct tm tm;
	utc_mkdate(when, &tm);

	// Format: yyyy/mm/dd-Location (location truncated to 15 chars, special chars removed)
	QString location = QString::fromStdString(trip->location);
	if (location.isEmpty())
		location = "Trip";

	// Remove problematic characters (matching save-git.cpp logic)
	QString cleanLoc;
	for (int i = 0; i < location.length() && cleanLoc.length() < 15; i++) {
		QChar c = location[i];
		if (c != ',' && c != '.' && c != '/' && c != '\\' && c != ':')
			cleanLoc += c;
	}

	return QString("%1/%2/%3-%4")
		.arg(tm.tm_year, 4, 10, QChar('0'))
		.arg(tm.tm_mon + 1, 2, 10, QChar('0'))
		.arg(tm.tm_mday, 2, 10, QChar('0'))
		.arg(cleanLoc);
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

	// Parse the trip reference: yyyy/mm/dd-Location
	// Location is restricted to alphanumeric, spaces, and hyphens (max 50 chars)
	// This prevents path traversal and injection attacks
	static QRegularExpression re(R"((\d{4})/(\d{2})/(\d{2})-([\w\s-]{1,50}))");
	QRegularExpressionMatch match = re.match(tripRef);

	if (!match.hasMatch())
		return nullptr;

	int year = match.captured(1).toInt();
	int month = match.captured(2).toInt();
	int day = match.captured(3).toInt();
	QString locationPart = match.captured(4);

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

	// Search for trip that starts on this day and has matching location prefix
	for (auto &trip : divelog.trips) {
		timestamp_t tripDate = trip->date();
		if (tripDate >= dayStart && tripDate < dayEnd) {
			// Check if location matches (prefix match)
			QString tripLoc = QString::fromStdString(trip->location);
			// Remove problematic chars for comparison
			QString cleanLoc;
			for (int i = 0; i < tripLoc.length() && cleanLoc.length() < 15; i++) {
				QChar c = tripLoc[i];
				if (c != ',' && c != '.' && c != '/' && c != '\\' && c != ':')
					cleanLoc += c;
			}
			if (cleanLoc == locationPart || (trip->location.empty() && locationPart == "Trip"))
				return trip.get();
		}
	}

	return nullptr;
}
