#ifndef DIVELIST_H
#define DIVELIST_H

#include <gtk/gtk.h>

struct dive;

extern GtkWidget *dive_list_create(void);
extern void dive_list_update_dives(void);
extern void update_dive_list_units(void);
extern void flush_divelist(struct dive *);

#endif
