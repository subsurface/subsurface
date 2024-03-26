// SPDX-License-Identifier: GPL-2.0
#ifndef QTHELPER_H
#define QTHELPER_H

#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include "core/pref.h"
#include "subsurface-time.h"

struct picture;
struct dive_trip;
struct xml_params;

// 1) Types

enum watertypes {FRESHWATER, BRACKISHWATER, EN13319WATER, SALTWATER, DC_WATERTYPE};

// 2) Functions visible only to C++ parts

#ifdef __cplusplus

#include <QString>
#include <optional>
#include <string>
#include "core/gettextfromc.h"
class QImage;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define SKIP_EMPTY Qt::SkipEmptyParts
#else
#define SKIP_EMPTY QString::SkipEmptyParts
#endif

QString weight_string(int weight_in_grams);
QString distance_string(int distanceInMeters);
bool gpsHasChanged(struct dive *dive, struct dive *master, const QString &gps_text, bool *parsed_out = 0);
QString get_gas_string(struct gasmix gas);
QStringList get_dive_gas_list(const struct dive *d);
QStringList stringToList(const QString &s);
void read_hashes();
void write_hashes();
QString thumbnailFileName(const QString &filename);
void learnPictureFilename(const QString &originalName, const QString &localName);
QString localFilePath(const QString &originalFilename);
std::optional<std::string> getCloudURL(); // move to prefs.h, probably.
bool parseGpsText(const QString &gps_text, double *latitude, double *longitude);
void init_proxy();
QStringList getWaterTypesAsString();
extern const QStringList videoExtensionsList;
QStringList mediaExtensionFilters();
QStringList imageExtensionFilters();
QStringList videoExtensionFilters();
char *copy_qstring(const QString &);
QString get_depth_string(depth_t depth, bool showunit = false, bool showdecimal = true);
QString get_depth_string(int mm, bool showunit = false, bool showdecimal = true);
QString get_depth_unit(bool metric);
QString get_depth_unit(); // use preferences unit
QString get_weight_string(weight_t weight, bool showunit = false);
QString get_weight_unit(bool metric);
QString get_weight_unit(); // use preferences unit
QString get_temperature_string(temperature_t temp, bool showunit = false);
QString get_temp_unit(bool metric);
QString get_temp_unit(); // use preferences unit
QString get_volume_string(volume_t volume, bool showunit = false);
QString get_volume_string(int mliter, bool showunit = false);
QString get_volume_unit(bool metric);
QString get_volume_unit(); // use preferences unit
QString get_pressure_string(pressure_t pressure, bool showunit = false);
QString get_salinity_string(int salinity);
QString get_water_type_string(int salinity);
QString getSubsurfaceDataPath(QString folderToFind);
QString getPrintingTemplatePathUser();
QString getPrintingTemplatePathBundle();
int gettimezoneoffset();
QDateTime timestampToDateTime(timestamp_t when);
timestamp_t dateTimeToTimestamp(const QDateTime &t);
int parseDurationToSeconds(const QString &text);
int parseLengthToMm(const QString &text);
int parseTemperatureToMkelvin(const QString &text);
int parseWeightToGrams(const QString &text);
int parsePressureToMbar(const QString &text);
int parseGasMixO2(const QString &text);
int parseGasMixHE(const QString &text);
QString render_seconds_to_string(int seconds);
QString get_dive_duration_string(timestamp_t when, QString hoursText, QString minutesText, QString secondsText = gettextFromC::tr("sec"), QString separator = ":", bool isFreeDive = false);
QString get_dive_surfint_string(timestamp_t when, QString daysText, QString hoursText, QString minutesText, QString separator = " ", int maxdays = 4);
QString get_dive_date_string(timestamp_t when);
std::string get_dive_date_c_string(timestamp_t when);
QString get_first_dive_date_string();
QString get_last_dive_date_string();
QString get_short_dive_date_string(timestamp_t when);
QString getUiLanguage();
void initUiLanguage();
QLocale getLocale();
QVector<QPair<QString, int>> selectedDivesGasUsed();
QString getUserAgent();
QString printGPSCoords(const location_t *loc);
std::string printGPSCoordsC(const location_t *loc);
std::vector<int> get_cylinder_map_for_remove(int count, int n);
std::vector<int> get_cylinder_map_for_add(int count, int n);
std::string get_current_date();

extern QString (*changesCallback)();
void uiNotification(const QString &msg);
std::string get_changes_made();
std::string subsurface_user_agent();
std::string get_file_name(const char *fileName);
std::string move_away(const std::string &path);

#if defined __APPLE__
#define TITLE_OR_TEXT(_t, _m) "", _t + "\n" + _m
#else
#define TITLE_OR_TEXT(_t, _m) _t, _m
#endif

#endif

// 3) Functions visible to C and C++

#ifdef __cplusplus
extern "C" {
#endif

struct git_info;

bool canReachCloudServer(struct git_info *);
void updateWindowTitle();
void subsurface_mkdir(const char *dir);
void copy_image_and_overwrite(const char *cfileName, const char *path, const char *cnewName);
const char *local_file_path(struct picture *picture);
char *hashfile_name_string();
enum deco_mode decoMode(bool in_planner);
void parse_seabear_header(const char *filename, struct xml_params *params);
time_t get_dive_datetime_from_isostring(char *when);
void print_qt_versions();
void lock_planner();
void unlock_planner();
xsltStylesheetPtr get_stylesheet(const char *name);
weight_t string_to_weight(const char *str);
depth_t string_to_depth(const char *str);
pressure_t string_to_pressure(const char *str);
volume_t string_to_volume(const char *str, pressure_t workp);
fraction_t string_to_fraction(const char *str);
void emit_reset_signal();

#ifdef __cplusplus
}
#endif

#endif // QTHELPER_H
