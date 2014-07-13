#ifndef QTHELPER_H
#define QTHELPER_H

#include <QMultiMap>
#include <QString>
#include <stdint.h>
#include "dive.h"
#include "divelist.h"
#include <QTranslator>

// global pointers for our translation
extern QTranslator *qtTranslator, *ssrfTranslator;

QString weight_string(int weight_in_grams);
bool gpsHasChanged(struct dive *dive, struct dive *master, const QString &gps_text, bool *parsed_out = 0);
QString printGPSCoords(int lat, int lon);
QList<int> getDivesInTrip(dive_trip_t *trip);
QString gasToStr(struct gasmix gas);

#endif // QTHELPER_H
