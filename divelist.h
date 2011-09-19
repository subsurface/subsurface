#ifndef DIVELIST_H
#define DIVELIST_H

#include <gtk/gtk.h>

struct DiveList {
	GtkWidget    *tree_view;
	GtkWidget    *container_widget;
	GtkListStore *model;
	GtkTreeViewColumn *date, *depth, *duration, *location;
	GtkTreeViewColumn *temperature, *nitrox, *sac;
};

struct dive;

extern struct DiveList dive_list;
extern struct DiveList dive_list_create(void);
extern void dive_list_update_dives(struct DiveList);
extern void update_dive_list_units(struct DiveList *);
extern void flush_divelist(struct DiveList *, struct dive *);

#endif
