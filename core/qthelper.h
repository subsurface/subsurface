// SPDX-License-Identifier: GPL-2.0
#ifndef QTHELPER_H
#define QTHELPER_H

#include "core/pref.h"
#include "subsurface-time.h"
#include <optional>
#include <string>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <QDateTime>
#include <QLocale>
#include <QPair>
#include <QString>

struct picture;
struct dive_trip;
struct xml_params;
struct git_info;
struct divecomputer;
class QImage;

enum watertypes {FRESHWATER, BRACKISHWATER, EN13319WATER, SALTWATER, DC_WATERTYPE};

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
#define SKIP_EMPTY Qt::SkipEmptyParts
#else
#define SKIP_EMPTY QString::SkipEmptyParts
#endif

bool gpsHasChanged(struct dive *dive, struct dive *master, const QString &gps_text, bool *parsed_out = 0);
QString get_gas_string(struct gasmix gas);
QString get_dive_gas(const struct dive *d, int dcNr, int cylinderId);
std::vector<std::pair<int, QString>> get_dive_gas_list(const struct dive *d, int dcNr, bool showOnlyAppropriate);
std::vector<std::pair<int, QString>> get_tank_sensor_list(const divecomputer &dc);
QStringList stringToList(const QString &s);
void read_hashes();
void write_hashes();
QString thumbnailFileName(const QString &filename);
void learnPictureFilename(const QString &originalName, const QString &localName);
QString localFilePath(const QString &originalFilename);
std::optional<std::string> getCloudURL(); // move to prefs.h, probably.
bool parseGpsText(const QString &gps_text, double *latitude, double *longitude);
void init_proxy();
QStringList mediaExtensionFilters();
QStringList imageExtensionFilters();
QString getSubsurfaceDataPath(QString folderToFind);
QString getPrintingTemplatePathUser();
QString getPrintingTemplatePathBundle();
QDateTime timestampToDateTime(timestamp_t when);
timestamp_t dateTimeToTimestamp(const QDateTime &t);
int parseDurationToSeconds(const QString &text);
int parseLengthToMm(const QString &text);
int parseTemperatureToMkelvin(const QString &text);
int parsePressureToMbar(const QString &text);
int parseGasMixO2(const QString &text);
int parseGasMixHE(const QString &text);
QString render_seconds_to_string(int seconds);
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
std::string get_file_name(const std::string &fileName);
std::string move_away(const std::string &path);

#if defined __APPLE__
#define TITLE_OR_TEXT(_t, _m) "", _t + "\n" + _m
#else
#define TITLE_OR_TEXT(_t, _m) _t, _m
#endif

bool canReachCloudServer(struct git_info *);
void updateWindowTitle();
void subsurface_mkdir(const char *dir);
std::string local_file_path(const struct picture &picture);
std::string hashfile_name();
enum deco_mode decoMode(bool in_planner);
void parse_seabear_header(const char *filename, struct xml_params *params);
time_t get_dive_datetime_from_isostring(const char *when);
void lock_planner();
void unlock_planner();
xsltStylesheetPtr get_stylesheet(const char *name);
weight_t string_to_weight(const char *str);
depth_t string_to_depth(const char *str);
pressure_t string_to_pressure(const char *str);
volume_t string_to_volume(const char *str, pressure_t workp);
fraction_t string_to_fraction(const char *str);
void emit_reset_signal();

#endif // QTHELPER_H
