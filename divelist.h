#ifndef DIVELIST_H
#define DIVELIST_H

#include <gtk/gtk.h>

struct DiveList {
	GtkWidget    *tree_view;
	GtkWidget    *container_widget;
	GtkListStore *model;
	GtkTreeViewColumn *date, *depth, *duration;
	GtkTreeViewColumn *temperature, *nitrox, *sac;
};

extern struct DiveList dive_list;
extern struct DiveList dive_list_create(void);
extern void dive_list_update_dives(struct DiveList);
extern void update_dive_list_units(struct DiveList *);

#endif
