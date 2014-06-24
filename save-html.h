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

void export_HTML(const char *file_name, const bool selected_only, const bool list_only);

#ifdef __cplusplus
}
#endif

#endif
