#ifdef __cplusplus
extern "C" {
#endif

extern void webservice_download_dialog(void);
extern gboolean webservice_request_user_xml(const gchar *, gchar **, guint *, guint *);
extern int divelogde_upload(char *fn, char **error);

#ifdef __cplusplus
}
#endif
