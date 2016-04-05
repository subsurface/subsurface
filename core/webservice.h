#ifndef WEBSERVICE_H
#define WEBSERVICE_H

#ifdef __cplusplus
extern "C" {
#endif

//extern void webservice_download_dialog(void);
//extern bool webservice_request_user_xml(const gchar *, gchar **, unsigned int *, unsigned int *);
extern int divelogde_upload(char *fn, char **error);
extern unsigned int download_dialog_parse_response(char *xmldata, unsigned int len);

enum {
	DD_STATUS_OK,
	DD_STATUS_ERROR_CONNECT,
	DD_STATUS_ERROR_ID,
	DD_STATUS_ERROR_PARSE,
};


#ifdef __cplusplus
}
#endif
#endif // WEBSERVICE_H
