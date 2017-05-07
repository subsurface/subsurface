// SPDX-License-Identifier: GPL-2.0
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
extern "C" const char *get_current_date();
QList<int> getDivesInTrip(dive_trip_t *trip);
QString get_gas_string(struct gasmix gas);
QString get_divepoint_gas_string(struct dive *d, const divedatapoint& dp);
void read_hashes();
void write_hashes();
void updateHash(struct picture *picture);
QByteArray hashFile(const QString filename);
void learnImages(const QDir dir, int max_recursions);
void add_hash(const QString filename, QByteArray hash);
void hashPicture(struct picture *picture);
QString localFilePath(const QString originalFilename);
QString fileFromHash(const char *hash);
void learnHash(struct picture *picture, QByteArray hash);
extern "C" void cache_picture(struct picture *picture);
weight_t string_to_weight(const char *str);
depth_t string_to_depth(const char *str);
pressure_t string_to_pressure(const char *str);
volume_t string_to_volume(const char *str, pressure_t workp);
fraction_t string_to_fraction(const char *str);
int getCloudURL(QString &filename);
bool parseGpsText(const QString &gps_text, double *latitude, double *longitude);
QByteArray getCurrentAppState();
void setCurrentAppState(QByteArray state);
extern "C" bool in_planner();
extern "C" enum deco_mode decoMode();
extern "C" void subsurface_mkdir(const char *dir);
void init_proxy();
QString getUUID();
char *intdup(int index);

#endif // QTHELPER_H
