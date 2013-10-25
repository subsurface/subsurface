#include "subsurfacewebservices.h"
#include "../webservice.h"

#include <libxml/parser.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDebug>
#include <QSettings>
#include <qdesktopservices.h>

#include "../dive.h"
#include "../divelist.h"

struct dive_table gps_location_table;
static bool merge_locations_into_dives(void);

WebServices::WebServices(QWidget* parent, Qt::WindowFlags f): QDialog(parent, f)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
	connect(ui.download, SIGNAL(clicked(bool)), this, SLOT(startDownload()));
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

}

void WebServices::hidePassword()
{
	ui.password->hide();
	ui.passLabel->hide();
}

void WebServices::hideUpload()
{
	ui.upload->hide();
}

SubsurfaceWebServices* SubsurfaceWebServices::instance()
{
	static SubsurfaceWebServices *self = new SubsurfaceWebServices();
	self->setAttribute(Qt::WA_QuitOnClose, false);
	return self;
}

SubsurfaceWebServices::SubsurfaceWebServices(QWidget* parent, Qt::WindowFlags f)
{
	QSettings s;
	ui.userID->setText(s.value("webservice_uid").toString());
	hidePassword();
	hideUpload();
}

static void clear_table(struct dive_table *table)
{
	int i;
	for (i = 0; i < table->nr; i++)
		free(table->dives[i]);
	table->nr = 0;
}

void SubsurfaceWebServices::buttonClicked(QAbstractButton* button)
{
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	switch(ui.buttonBox->buttonRole(button)){
		case QDialogButtonBox::ApplyRole:{
			clear_table(&gps_location_table);
			QByteArray url = tr("Webservice").toLocal8Bit();
			parse_xml_buffer(url.data(), downloadedData.data(), downloadedData.length(), &gps_location_table, NULL, NULL);

			/* now merge the data in the gps_location table into the dive_table */
			if (merge_locations_into_dives()) {
				mark_divelist_changed(TRUE);
			}

			/* store last entered uid in config */
			QSettings s;
			s.setValue("subsurface_webservice_uid", ui.userID->text());
			s.sync();
			hide();
			close();
		}
		break;
		case QDialogButtonBox::RejectRole:
			// we may want to clean up after ourselves, but this
			// makes Subsurface throw a SIGSEGV...
			// manager->deleteLater();
			reply->deleteLater();
			ui.progressBar->setMaximum(1);
			break;
		case QDialogButtonBox::HelpRole:
			QDesktopServices::openUrl(QUrl("http://api.hohndel.org"));
			break;
		default:
			break;
	}
}

void SubsurfaceWebServices::startDownload()
{
	QUrl url("http://api.hohndel.org/api/dive/get/");
	url.setQueryItems( QList<QPair<QString,QString> >() << qMakePair(QString("login"), ui.userID->text()));

	manager = new QNetworkAccessManager(this);
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("Accept", "text/xml");
	reply = manager->get(request);
	ui.status->setText(tr("Wait a bit untill we have something..."));
	ui.progressBar->setRange(0,0); // this makes the progressbar do an 'infinite spin'
	ui.download->setEnabled(false);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

	connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
			this, SLOT(downloadError(QNetworkReply::NetworkError)));
}

void SubsurfaceWebServices::downloadFinished()
{
	ui.progressBar->setRange(0,1);
	downloadedData = reply->readAll();

	ui.download->setEnabled(true);
	ui.status->setText(tr("Download Finished"));

	uint resultCode = download_dialog_parse_response(downloadedData);
	setStatusText(resultCode);
	if (resultCode == DD_STATUS_OK){
		ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
	}
	manager->deleteLater();
	reply->deleteLater();
}

void SubsurfaceWebServices::downloadError(QNetworkReply::NetworkError error)
{
	ui.download->setEnabled(true);
	ui.progressBar->setRange(0,1);
	ui.status->setText(QString::number((int)QNetworkRequest::HttpStatusCodeAttribute));
	manager->deleteLater();
	reply->deleteLater();
}

void SubsurfaceWebServices::setStatusText(int status)
{
	QString text;
	switch (status)	{
	case DD_STATUS_ERROR_CONNECT: text = tr("Connection Error: ");		break;
	case DD_STATUS_ERROR_ID:	  text = tr("Invalid user identifier!"); break;
	case DD_STATUS_ERROR_PARSE:	  text = tr("Cannot parse response!");	break;
	case DD_STATUS_OK:			  text = tr("Download Success!"); break;
	}
	ui.status->setText(text);
}

/* requires that there is a <download> or <error> tag under the <root> tag */
void SubsurfaceWebServices::download_dialog_traverse_xml(xmlNodePtr node, unsigned int *download_status)
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

unsigned int SubsurfaceWebServices::download_dialog_parse_response(const QByteArray& xml)
{
	xmlNodePtr root;
	xmlDocPtr doc = xmlParseMemory(xml.data(), xml.length());
	unsigned int status = DD_STATUS_ERROR_PARSE;

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

static bool is_automatic_fix(struct dive *gpsfix)
{
	if (gpsfix && gpsfix->location &&
	    (!strcmp(gpsfix->location, "automatic fix") ||
	     !strcmp(gpsfix->location, "Auto-created dive")))
		return TRUE;
	return FALSE;
}

#define SAME_GROUP 6 * 3600   // six hours

static bool merge_locations_into_dives(void)
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
