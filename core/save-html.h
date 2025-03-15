// SPDX-License-Identifier: GPL-2.0
#ifndef HTML_SAVE_H
#define HTML_SAVE_H

#include "membuffer.h"

struct dive;

void put_HTML_date(struct membuffer *b, const struct dive &dive, const char *pre, const char *post);
void put_HTML_depth(struct membuffer *b, const struct dive &dive, const char *pre, const char *post);
void put_HTML_airtemp(struct membuffer *b, const struct dive &dive, const char *pre, const char *post);
void put_HTML_watertemp(struct membuffer *b, const struct dive &dive, const char *pre, const char *post);
void put_HTML_time(struct membuffer *b, const struct dive &dive, const char *pre, const char *post);
void put_HTML_notes(struct membuffer *b, const struct dive &dive, const char *pre, const char *post);
void put_HTML_quoted(struct membuffer *b, const char *text);
void put_HTML_pressure_units(struct membuffer *b, pressure_t pressure, const char *pre, const char *post);
void put_HTML_weight_units(struct membuffer *b, unsigned int grams, const char *pre, const char *post);
void put_HTML_volume_units(struct membuffer *b, unsigned int ml, const char *pre, const char *post);

void export_JS(const char *file_name, const char *photos_dir, const bool selected_only, const bool list_only);
void export_list_as_JS(struct membuffer *b, const char *photos_dir, bool selected_only, const bool list_only);
void export_JSON(const char *file_name, const bool selected_only, const bool list_only);
void export_list_as_JSON(struct membuffer *b, const bool selected_only, const bool list_only);
void export_translation(const char *file_name);

#endif
