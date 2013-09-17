/*
 * info.h
 *
 * logic functions used from info-gtk.c
 */
#ifndef INFO_H
#define INFO_H

#ifdef __cplusplus
extern "C" {
#endif

extern gboolean gps_changed(struct dive *dive, struct dive *master, const char *gps_text);
extern void print_gps_coordinates(char *buffer, int len, int lat, int lon);
extern void save_equipment_data(struct dive *dive);
extern void update_equipment_data(struct dive *dive, struct dive *master);
extern void update_time_depth(struct dive *dive, struct dive *edited);
extern const char *get_window_title(struct dive *dive);
extern char *evaluate_string_change(const char *newstring, char **textp, const char *master);
extern int text_changed(const char *old, const char * /*new is a c++ keyword*/);
extern gboolean parse_gps_text(const char *gps_text, double *latitude, double *longitude);
extern int divename(char *buf, size_t size, struct dive *dive, char *trailer);

#ifdef __cplusplus
}
#endif

#endif
