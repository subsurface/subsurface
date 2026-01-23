// SPDX-License-Identifier: GPL-2.0
#ifndef CLI_DIVE_REF_H
#define CLI_DIVE_REF_H

#include <QString>

struct dive;
struct dive_trip;

// Parse a dive reference (git path format) and find the corresponding dive
// Example: "2024/06/15-Sat-10=30=00"
struct dive *findDiveByRef(const QString &diveRef);

// Parse a trip reference (git path format) and find the corresponding trip
// Example: "2024/06/15-Bonaire"
struct dive_trip *findTripByRef(const QString &tripRef);

// Generate the dive reference string for a dive
QString getDiveRef(const struct dive *d);

// Generate the trip reference string for a trip
QString getTripRef(const struct dive_trip *trip);

#endif // CLI_DIVE_REF_H
