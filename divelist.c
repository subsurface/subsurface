#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "dive.h"
#include "display.h"

static void selection_cb(GtkTreeSelection *selection, GtkTreeModel *model)
{
	GtkTreeIter iter;
	GValue value = {0, };

	if (!gtk_tree_selection_get_selected(selection, NULL, &iter))
		return;

	gtk_tree_model_get_value(model, &iter, 6, &value);
	selected_dive = g_value_get_int(&value);
	repaint_dive();
}

static void fill_dive_list(GtkListStore *store)
{
	int i;
	GtkTreeIter iter;

	for (i = 0; i < dive_table.nr; i++) {
		struct dive *dive = dive_table.dives[i];

		int len;
		char buffer[256], *datestr, *depth, *duration;
		struct tm *tm;
		
		tm = gmtime(&dive->when);
		len = snprintf(buffer, sizeof(buffer),
			"%02d.%02d.%02d %02d:%02d",
			tm->tm_mday, tm->tm_mon+1, tm->tm_year % 100,
			tm->tm_hour, tm->tm_min);
		datestr = malloc(len+1);
		memcpy(datestr, buffer, len+1);

		len = snprintf(buffer, sizeof(buffer),
		               "%d", to_feet(dive->maxdepth));
		depth = malloc(len + 1);
		memcpy(depth, buffer, len+1);
		
		len = snprintf(buffer, sizeof(buffer),
		               "%d", dive->duration.seconds / 60);
		duration = malloc(len + 1);
		memcpy(duration, buffer, len+1);

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter,
			0, datestr,
			1, dive->when,
			2, depth,
			3, dive->maxdepth,
			4, duration,
			5, dive->duration.seconds,
			6, i,
			-1);
	}
}

GtkWidget *create_dive_list(void)
{
	GtkListStore      *model;
	GtkWidget         *tree_view;
	GtkTreeSelection  *selection;
	GtkCellRenderer   *renderer;
	GtkTreeViewColumn *col;
	GtkWidget         *scroll_window;

	model = gtk_list_store_new(7,
				   G_TYPE_STRING, G_TYPE_INT,
	                           G_TYPE_STRING, G_TYPE_INT, 
	                           G_TYPE_STRING, G_TYPE_INT,
	                           G_TYPE_INT);
	tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(model));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree_view));

	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(selection), GTK_SELECTION_BROWSE);
	gtk_widget_set_size_request(tree_view, 200, 100);

	fill_dive_list(model);

	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "Date");
	gtk_tree_view_column_set_sort_column_id(col, 1);
	gtk_tree_view_column_set_resizable (col, TRUE);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);
	
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "ft");
	gtk_tree_view_column_set_sort_column_id(col, 3);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 2);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);
	
	renderer = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, "min");
	gtk_tree_view_column_set_sort_column_id(col, 5);
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, renderer, "text", 4);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), col);

	g_object_set(G_OBJECT(tree_view), "headers-visible", TRUE,
					  "search-column", 0,
					  "rules-hint", FALSE,
					  NULL);

	g_signal_connect(selection, "changed", G_CALLBACK(selection_cb), model);

	scroll_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll_window),
		               GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll_window), tree_view);

	return scroll_window;
}
