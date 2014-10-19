#ifndef HTML_SAVE_H
#define HTML_SAVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "dive.h"
#include "membuffer.h"

void put_HTML_date(struct membuffer *b, struct dive *dive, const char *pre, const char *post);
void put_HTML_airtemp(struct membuffer *b, struct dive *dive, const char *pre, const char *post);
void put_HTML_watertemp(struct membuffer *b, struct dive *dive, const char *pre, const char *post);
void put_HTML_time(struct membuffer *b, struct dive *dive, const char *pre, const char *post);
void put_HTML_notes(struct membuffer *b, struct dive *dive, const char *pre, const char *post);
void put_HTML_quoted(struct membuffer *b, const char *text);
void put_HTML_pressure_units(struct membuffer *b, pressure_t pressure, const char *pre, const char *post);
void put_HTML_weight_units(struct membuffer *b, unsigned int grams, const char *pre, const char *post);
void put_HTML_volume_units(struct membuffer *b, unsigned int ml, const char *pre, const char *post);

void export_HTML(const char *file_name, const char *photos_dir, const bool selected_only, const bool list_only);
void export_list(struct membuffer *b, const char *photos_dir, bool selected_only, const bool list_only);

void export_translation(const char *file_name);

extern void copy_image_and_overwrite(const char *cfileName, const char *cnewName);
extern char *get_file_name(const char *fileName);
#ifdef __cplusplus
}
#endif

#endif
