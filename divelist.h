#ifndef DIVELIST_H
#define DIVELIST_H

#include <gtk/gtk.h>

struct DiveList {
	GtkWidget    *tree_view;
	GtkWidget    *container_widget;
	GtkListStore *model;
	GtkTreeViewColumn *date, *depth, *duration;
};

extern int selected_dive;
#define current_dive (get_dive(selected_dive))

extern struct DiveList dive_list_create(void);
extern void dive_list_update_dives(struct DiveList);

#endif
