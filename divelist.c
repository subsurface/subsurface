#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"
#include "display.h"

static GtkTreeModel *fill_dive_list(void)
{
	int i;
	GtkListStore *store;
	GtkTreeIter iter;

	store = gtk_list_store_new(1, G_TYPE_STRING);

	for (i = 0; i < dive_table.nr; i++) {
		struct dive *dive = dive_table.dives[i];

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			0, dive->name,
			-1);
	}

	return GTK_TREE_MODEL(store);
}

GtkWidget *create_dive_list(void)
{
	GtkWidget *list;
	GtkCellRenderer *renderer;
	GtkTreeModel *model;
	GtkWidget *scrolled_window;

	list = gtk_tree_view_new();

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_insert_column_with_attributes(GTK_TREE_VIEW(list),
		-1, "Dive", renderer, "text", 0, NULL);

	model = fill_dive_list();
	gtk_tree_view_set_model(GTK_TREE_VIEW(list), model);
	g_object_unref(model);

	/* Scrolled window for the list goes into the vbox.. */
	scrolled_window=gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_usize(scrolled_window, 150, 350);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_widget_show(scrolled_window);

	/* .. and connect it to the scrolled window */
	gtk_container_add(GTK_CONTAINER(scrolled_window), list);

	return scrolled_window;
}
