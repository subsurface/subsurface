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
QString distance_string(int distanceInMeters);
bool gpsHasChanged(struct dive *dive, struct dive *master, const QString &gps_text, bool *parsed_out = 0);
extern "C" const char *printGPSCoords(int lat, int lon);
QList<int> getDivesInTrip(dive_trip_t *trip);
QString get_gas_string(struct gasmix gas);
QString get_divepoint_gas_string(const divedatapoint& dp);
void read_hashes();
void write_hashes();
void updateHash(struct picture *picture);
QByteArray hashFile(const QString filename);
void learnImages(const QDir dir, int max_recursions, bool recursed);
void add_hash(const QString filename, QByteArray hash);
QString localFilePath(const QString originalFilename);
QString fileFromHash(char *hash);
void learnHash(struct picture *picture, QByteArray hash);
weight_t string_to_weight(const char *str);
depth_t string_to_depth(const char *str);
pressure_t string_to_pressure(const char *str);
volume_t string_to_volume(const char *str, pressure_t workp);
fraction_t string_to_fraction(const char *str);
int getCloudURL(QString &filename);
void loadPreferences();
#endif // QTHELPER_H
