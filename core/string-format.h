// SPDX-License-Identifier: GPL-2.0
// Various functions that format data into QStrings or QStringLists
#ifndef STRING_FORMAT_H
#define STRING_FORMAT_H

#include <QStringList>

struct dive;

QString formatSac(const dive *d);
QString formatNotes(const dive *d);
QString format_gps_decimal(const dive *d);
QStringList formatGetCylinder(const dive *d);
QStringList formatStartPressure(const dive *d);
QStringList formatEndPressure(const dive *d);
QStringList formatFirstGas(const dive *d);
QStringList formatFullCylinderList();
QStringList formatCylinders(const dive *d);

#endif
