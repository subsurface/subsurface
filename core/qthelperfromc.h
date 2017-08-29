// SPDX-License-Identifier: GPL-2.0
#ifndef QTHELPERFROMC_H
#define QTHELPERFROMC_H

bool getProxyString(char **buffer);
bool canReachCloudServer();
void updateWindowTitle();
bool isCloudUrl(const char *filename);
void subsurface_mkdir(const char *dir);
char *get_file_name(const char *fileName);
void copy_image_and_overwrite(const char *cfileName, const char *path, const char *cnewName);
char *hashstring(char *filename);
bool picture_exists(struct picture *picture);
char *move_away(const char *path);
const char *local_file_path(struct picture *picture);
void savePictureLocal(struct picture *picture, const char *data, int len);
void cache_picture(struct picture *picture);
char *cloud_url();
char *hashfile_name_string();
char *picturedir_string();
const char *subsurface_user_agent();
enum deco_mode decoMode();
int parse_seabear_header(const char *filename, char **params, int pnr);
extern const char *get_current_date();
enum inertgas {N2, HE};
double cache_value(int tissue, int timestep, enum inertgas gas);
void cache_insert(int tissue, int timestep, enum inertgas gas, double value);
void print_qt_versions();
void lock_planner();
void unlock_planner();

#endif // QTHELPERFROMC_H
