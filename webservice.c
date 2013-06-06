#include <libintl.h>
#include <glib/gi18n.h>
#include <libsoup/soup.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include "dive.h"
#include "divelist.h"
#include "display-gtk.h"
#include "file.h"

struct dive_table gps_location_table;
static gboolean merge_locations_into_dives(void);

enum {
	DD_STATUS_OK,
	DD_STATUS_ERROR_CONNECT,
	DD_STATUS_ERROR_ID,
	DD_STATUS_ERROR_PARSE,
};

static const gchar *download_dialog_status_text(const gint status)
{
	switch (status)	{
	case DD_STATUS_ERROR_CONNECT:
		return _("Connection Error: ");
	case DD_STATUS_ERROR_ID:
		return _("Invalid user identifier!");
	case DD_STATUS_ERROR_PARSE:
		return _("Cannot parse response!");
	}
	return _("Download Success!");
}

/* provides a state of the download dialog contents and the downloaded xml */
struct download_dialog_state {
	GtkWidget *uid;
	GtkWidget *status;
	GtkWidget *apply;
	gchar *xmldata;
	guint xmldata_len;
};

/* this method uses libsoup as a backend. if there are some portability,
 * compatibility or speed issues, libcurl is a better choice. */
gboolean webservice_request_user_xml(const gchar *user_id,
	gchar **data,
	guint *len,
	guint *status_code)
{
	SoupMessage *msg;
	SoupSession *session;
	gboolean ret = FALSE;
	gchar url[256] = {0};

	session = soup_session_async_new();
	strcat(url, "http://api.hohndel.org/api/dive/get/?login=");
	strncat(url, user_id, sizeof(url) - strlen(url) - 1);
	msg = soup_message_new("GET", url);
	soup_message_headers_append(msg->request_headers, "Accept", "text/xml");
	soup_session_send_message(session, msg);
	if SOUP_STATUS_IS_SUCCESSFUL(msg->status_code) {
		*len = (guint)msg->response_body->length;
		*data = strdup((gchar *)msg->response_body->data);
		ret = TRUE;
	} else {
		*len = 0;
		*data = NULL;
	}
	*status_code = msg->status_code;
	soup_session_abort(session);
	g_object_unref(G_OBJECT(msg));
	g_object_unref(G_OBJECT(session));
	return ret;
}

/* requires that there is a <download> or <error> tag under the <root> tag */
static void download_dialog_traverse_xml(xmlNodePtr node, guint *download_status)
{
	xmlNodePtr cur_node;
	for (cur_node = node; cur_node; cur_node = cur_node->next) {
		if ((!strcmp((const char *)cur_node->name, (const char *)"download")) &&
			  (!strcmp((const char *)xmlNodeGetContent(cur_node), (const char *)"ok"))) {
			*download_status = DD_STATUS_OK;
			return;
		}	else if (!strcmp((const char *)cur_node->name, (const char *)"error")) {
			*download_status = DD_STATUS_ERROR_ID;
			return;
		}
	}
}

static guint download_dialog_parse_response(gchar *xmldata, guint len)
{
	xmlNodePtr root;
	xmlDocPtr doc = xmlParseMemory(xmldata, len);
	guint status = DD_STATUS_ERROR_PARSE;

	if (!doc)
		return DD_STATUS_ERROR_PARSE;
	root = xmlDocGetRootElement(doc);
	if (!root) {
		status = DD_STATUS_ERROR_PARSE;
		goto end;
	}
	if (root->children)
		download_dialog_traverse_xml(root->children, &status);
	end:
		xmlFreeDoc(doc);
		return status;
}

static void download_dialog_connect_cb(GtkWidget *w, gpointer data)
{
	struct download_dialog_state *state = (struct download_dialog_state *)data;
	const gchar *uid = gtk_entry_get_text(GTK_ENTRY(state->uid));
	guint len, status_connect, status_xml;
	gchar *xmldata;
	gboolean ret;
	gchar err[256] = {0};

	gtk_label_set_text(GTK_LABEL(state->status), _("Connecting..."));
	gtk_widget_set_sensitive(state->apply, FALSE);
	ret = webservice_request_user_xml(uid, &xmldata, &len, &status_connect);
	if (ret) {
		status_xml = download_dialog_parse_response(xmldata, len);
		gtk_label_set_text(GTK_LABEL(state->status), download_dialog_status_text(status_xml));
		if (status_xml != DD_STATUS_OK)
			ret = FALSE;
	} else {
		snprintf(err, sizeof(err), "%s %u!", download_dialog_status_text(DD_STATUS_ERROR_CONNECT), status_connect);
		gtk_label_set_text(GTK_LABEL(state->status), err);
	}
	state->xmldata = xmldata;
	state->xmldata_len = len;
	gtk_widget_set_sensitive(state->apply, ret);
}

static void download_dialog_release_xml(struct download_dialog_state *state)
{
	if (state->xmldata)
		free((void *)state->xmldata);
}

static void clear_table(struct dive_table *table)
{
	int i;
	for (i = 0; i < table->nr; i++)
		free(table->dives[i]);
	table->nr = 0;
}

static void download_dialog_response_cb(GtkDialog *d, gint response, gpointer data)
{
	struct download_dialog_state *state = (struct download_dialog_state *)data;
	switch (response) {
	case GTK_RESPONSE_HELP:
		/* open webservice api page */
		subsurface_launch_for_uri("http://api.hohndel.org/");
		break;
	case GTK_RESPONSE_ACCEPT:
		/* apply download */
		clear_table(&gps_location_table);
		parse_xml_buffer(_("Webservice"), state->xmldata, state->xmldata_len, &gps_location_table, NULL);
		/* now merge the data in the gps_location table into the dive_table */
		if (merge_locations_into_dives()) {
			mark_divelist_changed(TRUE);
#if USE_GTK_UI
			dive_list_update_dives();
#endif
		}
		/* store last entered uid in config */
		subsurface_set_conf("webservice_uid", gtk_entry_get_text(GTK_ENTRY(state->uid)));
	default:
	case GTK_RESPONSE_DELETE_EVENT:
		gtk_widget_destroy(GTK_WIDGET(d));
		download_dialog_release_xml(state);
		free(state);
	}
}

static gboolean is_automatic_fix(struct dive *gpsfix)
{
	if (gpsfix && gpsfix->location &&
	    (!strcmp(gpsfix->location, "automatic fix") ||
	     !strcmp(gpsfix->location, "Auto-created dive")))
		return TRUE;
	return FALSE;
}

#define SAME_GROUP 6 * 3600   // six hours

/* returns TRUE if dive_table was changed */
static gboolean merge_locations_into_dives(void)
{
	int i, nr = 0, changed = 0;
	struct dive *gpsfix, *last_named_fix = NULL, *dive;

	sort_table(&gps_location_table);

	for_each_gps_location(i, gpsfix) {
		if (is_automatic_fix(gpsfix)) {
			dive = find_dive_including(gpsfix->when);
			if (dive && !dive_has_gps_location(dive)) {
#if DEBUG_WEBSERVICE
				struct tm tm;
				utc_mkdate(gpsfix->when, &tm);
				printf("found dive named %s @ %04d-%02d-%02d %02d:%02d:%02d\n",
					gpsfix->location,
					tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
					tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif
				changed++;
				copy_gps_location(gpsfix, dive);
			}
		} else {
			if (last_named_fix && dive_within_time_range(last_named_fix, gpsfix->when, SAME_GROUP)) {
				nr++;
			} else {
				nr = 1;
				last_named_fix = gpsfix;
			}
			dive = find_dive_n_near(gpsfix->when, nr, SAME_GROUP);
			if (dive) {
				if (!dive_has_gps_location(dive)) {
					copy_gps_location(gpsfix, dive);
					changed++;
				}
				if (!dive->location) {
					dive->location = strdup(gpsfix->location);
					changed++;
				}
			} else {
				struct tm tm;
				utc_mkdate(gpsfix->when, &tm);
#if DEBUG_WEBSERVICE
				printf("didn't find dive matching gps fix named %s @ %04d-%02d-%02d %02d:%02d:%02d\n",
					gpsfix->location,
					tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday,
					tm.tm_hour, tm.tm_min, tm.tm_sec);
#endif
			}
		}
	}
	return changed > 0;
}

void webservice_download_dialog(void)
{
	const guint pad = 6;
	/* user entered value should be stored in the config */
	const gchar *current_uid = subsurface_get_conf("webservice_uid");
	GtkWidget *dialog, *vbox, *status, *info, *uid;
	GtkWidget *frame_uid, *frame_status, *download, *image, *apply;
	struct download_dialog_state *state = calloc(1, sizeof(struct download_dialog_state));
	gboolean has_previous_uid = TRUE;

	if (!current_uid) {
		current_uid = "";
		has_previous_uid = FALSE;
	}

	dialog = gtk_dialog_new_with_buttons(_("Download From Web Service"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_APPLY,
		GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_REJECT,
		GTK_STOCK_HELP,
		GTK_RESPONSE_HELP,
		NULL);

	apply = gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
	gtk_widget_set_sensitive(apply, FALSE);

	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	info = gtk_label_new(_("Enter a user identifier and press 'Download'."
				 " Once the download is complete you can press 'Apply'"
				 " if you wish to apply the changes."));
	gtk_label_set_line_wrap(GTK_LABEL(info), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), info, FALSE, TRUE, 0);
	gtk_misc_set_padding(GTK_MISC(info), pad, pad);

	frame_uid = gtk_frame_new(_("User Identifier"));
	gtk_box_pack_start(GTK_BOX(vbox), frame_uid, FALSE, TRUE, pad);
	uid = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(frame_uid), uid);
	gtk_entry_set_max_length(GTK_ENTRY(uid), 30);
	gtk_entry_set_text(GTK_ENTRY(uid), current_uid);

	download = gtk_button_new_with_label(_(" Download"));
	image = gtk_image_new_from_stock(GTK_STOCK_CONNECT, GTK_ICON_SIZE_MENU);
	gtk_button_set_image(GTK_BUTTON(download), image);
	gtk_box_pack_start(GTK_BOX(vbox), download, FALSE, TRUE, pad);
	g_signal_connect(download, "clicked", G_CALLBACK(download_dialog_connect_cb), (gpointer)state);

	frame_status = gtk_frame_new(_("Status"));
	status = gtk_label_new(_("Idle"));
	gtk_box_pack_start(GTK_BOX(vbox), frame_status, FALSE, TRUE, pad);
	gtk_container_add(GTK_CONTAINER(frame_status), status);
	gtk_misc_set_padding(GTK_MISC(status), pad, pad);

	state->uid = uid;
	state->status = status;
	state->apply = apply;

	g_signal_connect(dialog, "response", G_CALLBACK(download_dialog_response_cb), (gpointer)state);
	gtk_widget_show_all(dialog);
	if (has_previous_uid)
		free((void *)current_uid);
}

static gboolean divelogde_dialog(const char **user, const char **pass)
{
	GtkWidget *dialog, *vbox, *info, *frame_user, *frame_pass, *uid, *pwd;
	gboolean ret = FALSE;

	*user = subsurface_get_conf("divelogde_user");
	*pass = subsurface_get_conf("divelogde_pass");
	dialog = gtk_dialog_new_with_buttons(_("Upload to divelogs.de"),
		GTK_WINDOW(main_window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_APPLY,
		GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL,
		GTK_RESPONSE_REJECT,
		NULL);
	vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	info = gtk_label_new(_("Please enter your userid and password for divelogs.de. "
				"The selected dives will be added to your account"));
	gtk_label_set_line_wrap(GTK_LABEL(info), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), info, FALSE, TRUE, 0);
	gtk_misc_set_padding(GTK_MISC(info), 6, 6);

	frame_user = gtk_frame_new(_("User Identifier"));
	gtk_box_pack_start(GTK_BOX(vbox), frame_user, FALSE, TRUE, 6);
	uid = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(frame_user), uid);
	gtk_entry_set_max_length(GTK_ENTRY(uid), 40);
	gtk_entry_set_text(GTK_ENTRY(uid), *user ?: "");
	gtk_entry_set_activates_default(GTK_ENTRY(uid), TRUE);

	frame_pass = gtk_frame_new(_("Password"));
	gtk_box_pack_start(GTK_BOX(vbox), frame_pass, FALSE, TRUE, 6);
	pwd = gtk_entry_new();
	gtk_container_add(GTK_CONTAINER(frame_pass), pwd);
	gtk_entry_set_max_length(GTK_ENTRY(pwd), 40);
	gtk_entry_set_text(GTK_ENTRY(pwd), *pass ?: "");
	gtk_entry_set_visibility(GTK_ENTRY(pwd), FALSE);
	gtk_entry_set_activates_default(GTK_ENTRY(pwd), TRUE);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

	gtk_widget_show_all(dialog);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		free((void*)*user);
		free((void*)*pass);
		*user = strdup(gtk_entry_get_text(GTK_ENTRY(uid)));
		*pass = strdup(gtk_entry_get_text(GTK_ENTRY(pwd)));
		subsurface_set_conf("divelogde_user", *user);
		subsurface_set_conf("divelogde_pass", *pass);
		ret = TRUE;
	}
	gtk_widget_destroy(dialog);
	return ret;
}

int divelogde_upload(char *fn, char **error)
{
	SoupMessage *msg;
	SoupMultipart *multipart;
	SoupSession *session;
	SoupBuffer *sbuf;
	gboolean ret = FALSE;
#ifdef __MINGW32__
	/* for odd reasons I can't seem to get https connections to work
	 * with mingw32 when cross-building the Windows binaries... so fall
	 * back to http for now */
	char url[256] = "http://divelogs.de/DivelogsDirectImport.php";
#else
	char url[256] = "https://divelogs.de/DivelogsDirectImport.php";
#endif
	const char *pass = NULL;
	const char *user = NULL;
	struct memblock mem;

	if (readfile(fn, &mem) < 0)
		return ret;
	if (!divelogde_dialog(&user, &pass))
		return TRUE;
	sbuf = soup_buffer_new(SOUP_MEMORY_STATIC, mem.buffer, mem.size);
	session = soup_session_async_new();
	multipart = soup_multipart_new(SOUP_FORM_MIME_TYPE_MULTIPART);
	soup_multipart_append_form_string(multipart, "user", user);
	soup_multipart_append_form_string(multipart, "pass", pass);

	soup_multipart_append_form_file(multipart, "userfile", fn, NULL, sbuf);
	msg = soup_form_request_new_from_multipart(url, multipart);
	soup_message_headers_append(msg->request_headers, "Accept", "text/xml");
	soup_session_send_message(session, msg);
	if (SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
		*error = strdup(msg->response_body->data);
		ret = TRUE;
	} else {
		*error = strdup(msg->reason_phrase);
	}
	soup_session_abort(session);
	g_object_unref(G_OBJECT(msg));
	g_object_unref(G_OBJECT(session));
	return ret;
}
