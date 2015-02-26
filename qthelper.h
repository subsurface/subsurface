#ifndef QTHELPER_H
#define QTHELPER_H

#include <QMultiMap>
#include <QString>
#include <stdint.h>
#include "dive.h"
#include "divelist.h"
#include <QTranslator>
#include <QDir>

// global pointers for our translation
extern QTranslator *qtTranslator, *ssrfTranslator;

QString weight_string(int weight_in_grams);
bool gpsHasChanged(struct dive *dive, struct dive *master, const QString &gps_text, bool *parsed_out = 0);
extern "C" const char *printGPSCoords(int lat, int lon);
QList<int> getDivesInTrip(dive_trip_t *trip);
QString gasToStr(struct gasmix gas);
void read_hashes();
void write_hashes();
void updateHash(struct picture *picture);
QByteArray hashFile(const QString filename);
void add_hash(const QString filename, QByteArray &hash);
QString localFilePath(const QString originalFilename);
QString fileFromHash(char *hash);
#endif // QTHELPER_H
