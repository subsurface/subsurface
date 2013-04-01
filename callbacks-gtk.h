#define UNITCALLBACK(name, type, value)				\
static void name(GtkWidget *w, gpointer data) 			\
{								\
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))	\
		prefs.units.type = value;		\
	update_screen();					\
}

#define OPTIONCALLBACK(name, option)			\
static void name(GtkWidget *w, gpointer data)		\
{							\
	GtkWidget **entry = (GtkWidget**)data;			\
	option = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));	\
	update_screen();				\
	if (entry)					\
		gtk_widget_set_sensitive(*entry, option);\
}
