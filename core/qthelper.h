// SPDX-License-Identifier: GPL-2.0
#ifndef QTHELPER_H
#define QTHELPER_H

#include <QMultiMap>
#include <QString>
#include <stdint.h>
#include "dive.h"
#include "divelist.h"
#include "filelocation.h"
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
bool parseGpsText(const QString &gps_text, double *latitude, double *longitude);
QByteArray getCurrentAppState();
void setCurrentAppState(QByteArray state);
extern "C" bool in_planner();
extern "C" enum deco_mode decoMode();
extern "C" void subsurface_mkdir(const char *dir);
void init_proxy();
QString getUUID();
QStringList imageExtensionFilters();
char *intdup(int index);
extern "C" int parse_seabear_header(const char *filename, char **params, int pnr);
enum inertgas {N2, HE};
extern "C" double cache_value(int tissue, int timestep, enum inertgas gas);
extern "C" void cache_insert(int tissue, int timestep, enum inertgas gas, double value);
extern "C" void lock_planner();
extern "C" void unlock_planner();
void set_filename(const FileLocation &, bool force);
extern "C" void set_current_file_none();
extern "C" char *get_current_file_name();	// Caller must free() returned file name
extern "C" void get_cloud_info(struct git_state *state);
FileLocation getCloudLocation(bool isOffline);	// Return FileLocation::NONE if no cloud was set
#endif // QTHELPER_H
