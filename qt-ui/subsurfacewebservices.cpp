#include "subsurfacewebservices.h"
#include "../webservice.h"
#include "mainwindow.h"
#include <libxml/parser.h>
#include <zip.h>
#include <errno.h>

#include <QDir>
#include <QHttpMultiPart>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDebug>
#include <QSettings>
#include <QXmlStreamReader>
#include <qdesktopservices.h>

#include "../dive.h"
#include "../divelist.h"

#ifdef Q_OS_UNIX
#  include <unistd.h> // for dup(2)
#endif

struct dive_table gps_location_table;
static bool merge_locations_into_dives(void);

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

static void clear_table(struct dive_table *table)
{
	int i;
	for (i = 0; i < table->nr; i++)
		free(table->dives[i]);
	table->nr = 0;
}

static char *prepare_dives_for_divelogs(const bool selected)
{
	int i;
	struct dive *dive;
	FILE *f;
	char filename[PATH_MAX], *tempfile;
	size_t streamsize;
	char *membuf;
	xmlDoc *doc;
	xsltStylesheetPtr xslt = NULL;
	xmlDoc *transformed;
	struct zip_source *s[dive_table.nr];
	struct zip *zip;
	char *error = NULL;

	/* generate a random filename and create/open that file with zip_open */
	QString tempfileQ = QDir::tempPath() + "/import-" + QString::number(qrand() % 99999999) + ".dld";
	tempfile = tempfileQ.toLocal8Bit().data();
	zip = zip_open(tempfile, ZIP_CREATE, NULL);

	if (!zip) {
		fprintf(stderr, "divelog.de-upload: cannot open file as zip\n");
		return NULL;
	}
	if (!amount_selected) {
		fprintf(stderr, "divelog.de-upload: no dives selected\n");
		return NULL;
	}

	/* walk the dive list in chronological order */
	for (i = 0; i < dive_table.nr; i++) {
		dive = get_dive(i);
		if (!dive)
			continue;
		if (selected && !dive->selected)
			continue;
		f = tmpfile();
		if (!f) {
			fprintf(stderr, "divelog.de-upload: cannot create temp file\n");
			return NULL;
		}
		save_dive(f, dive);
		fseek(f, 0, SEEK_END);
		streamsize = ftell(f);
		rewind(f);
		membuf = (char *)malloc(streamsize + 1);
		if (!membuf || !fread(membuf, streamsize, 1, f)) {
			fprintf(stderr, "divelog.de-upload: memory error\n");
			return NULL;
		}
		membuf[streamsize] = 0;
		fclose(f);
		/*
		 * Parse the memory buffer into XML document and
		 * transform it to divelogs.de format, finally dumping
		 * the XML into a character buffer.
		 */
		doc = xmlReadMemory(membuf, strlen(membuf), "divelog", NULL, 0);
		if (!doc) {
			fprintf(stderr, "divelog.de-upload: xml error\n");
			continue;
		}
		free((void *)membuf);
		// this call is overriding our local variable tempfile! not a good sign!
		xslt = get_stylesheet("divelogs-export.xslt");
		if (!xslt) {
			fprintf(stderr, "divelog.de-upload: missing stylesheet\n");
			return NULL;
		}
		transformed = xsltApplyStylesheet(xslt, doc, NULL);
		xsltFreeStylesheet(xslt);
		xmlDocDumpMemory(transformed, (xmlChar **) &membuf, (int *)&streamsize);
		xmlFreeDoc(doc);
		xmlFreeDoc(transformed);
		/*
		 * Save the XML document into a zip file.
		 */
		snprintf(filename, PATH_MAX, "%d.xml", i + 1);
		s[i] = zip_source_buffer(zip, membuf, streamsize, 1);
		if (s[i]) {
			int64_t ret = zip_add(zip, filename, s[i]);
			if (ret == -1)
				fprintf(stderr, "divelog.de-upload: failed to include dive %d\n", i);
		}
	}
	zip_close(zip);
	/* let's call this again */
	tempfile = tempfileQ.toLocal8Bit().data();
	return tempfile;
}

WebServices::WebServices(QWidget* parent, Qt::WindowFlags f): QDialog(parent, f)
, reply(0)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
	connect(ui.download, SIGNAL(clicked(bool)), this, SLOT(startDownload()));
	connect(ui.upload, SIGNAL(clicked(bool)), this, SLOT(startUpload()));
	connect(&timeout, SIGNAL(timeout()), this, SLOT(downloadTimedOut()));
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	timeout.setSingleShot(true);
}

void WebServices::hidePassword()
{
	ui.password->hide();
	ui.passLabel->hide();
}

void WebServices::hideUpload()
{
	ui.upload->hide();
	ui.download->show();
}

void WebServices::hideDownload()
{
	ui.download->hide();
	ui.upload->show();
}

QNetworkAccessManager *WebServices::manager()
{
	static QNetworkAccessManager *manager = new QNetworkAccessManager(qApp);
	return manager;
}

void WebServices::downloadTimedOut()
{
	if (!reply)
		return;

	reply->deleteLater();
	reply = NULL;
	resetState();
	ui.status->setText(tr("Operation timed out"));
}

void WebServices::updateProgress(qint64 current, qint64 total)
{
	if (!reply)
		return;
	if (total == -1) {
		total = INT_MAX / 2 - 1;
	}
	if (total >= INT_MAX / 2) {
		// over a gigabyte!
		if (total >= Q_INT64_C(1) << 47) {
			total >>= 16;
			current >>= 16;
		}
		total >>= 16;
		current >>= 16;
	}
	ui.progressBar->setRange(0, total);
	ui.progressBar->setValue(current);
	ui.status->setText(tr("Transfering data..."));

	// reset the timer: 30 seconds after we last got any data
	timeout.start();
}

void WebServices::connectSignalsForDownload(QNetworkReply *reply)
{
	connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
		this, SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this,
		SLOT(updateProgress(qint64,qint64)));

	timeout.start(30000); // 30s
}

void WebServices::resetState()
{
	ui.download->setEnabled(true);
	ui.upload->setEnabled(true);
	ui.userID->setEnabled(true);
	ui.password->setEnabled(true);
	ui.progressBar->reset();
	ui.progressBar->setRange(0,1);
	ui.status->setText(QString());
}

// #
// #
// #		Subsurface Web Service Implementation.
// #
// #

SubsurfaceWebServices* SubsurfaceWebServices::instance()
{
	static SubsurfaceWebServices *self = new SubsurfaceWebServices(mainWindow());
	self->setAttribute(Qt::WA_QuitOnClose, false);
	return self;
}

SubsurfaceWebServices::SubsurfaceWebServices(QWidget* parent, Qt::WindowFlags f)
{
	QSettings s;
	ui.userID->setText(s.value("subsurface_webservice_uid").toString().toUpper());
	hidePassword();
	hideUpload();
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
		s.setValue("subsurface_webservice_uid", ui.userID->text().toUpper());
		s.sync();
		hide();
		close();
		resetState();
	}
		break;
	case QDialogButtonBox::RejectRole:
		// we may want to clean up after ourselves
		// reply->deleteLater();
		reply = NULL;
		resetState();
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
	url.addQueryItem("login", ui.userID->text().toUpper());

	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("Accept", "text/xml");
	reply = manager()->get(request);
	ui.status->setText(tr("Connecting..."));
	ui.progressBar->setRange(0,0); // this makes the progressbar do an 'infinite spin'
	ui.download->setEnabled(false);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	connectSignalsForDownload(reply);
}

void SubsurfaceWebServices::downloadFinished()
{
	if (!reply)
		return;

	ui.progressBar->setRange(0,1);
	downloadedData = reply->readAll();

	ui.download->setEnabled(true);
	ui.status->setText(tr("Download finished"));

	uint resultCode = download_dialog_parse_response(downloadedData);
	setStatusText(resultCode);
	if (resultCode == DD_STATUS_OK){
		ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
	}
	reply->deleteLater();
	reply = NULL;
}

void SubsurfaceWebServices::downloadError(QNetworkReply::NetworkError)
{
	resetState();
	ui.status->setText(tr("Download error: %1").arg(reply->errorString()));
	reply->deleteLater();
	reply = NULL;
}

void SubsurfaceWebServices::setStatusText(int status)
{
	QString text;
	switch (status)	{
	case DD_STATUS_ERROR_CONNECT:	text = tr("Connection Error: ");	break;
	case DD_STATUS_ERROR_ID:	text = tr("Invalid user identifier!");	break;
	case DD_STATUS_ERROR_PARSE:	text = tr("Cannot parse response!");	break;
	case DD_STATUS_OK:		text = tr("Download Success!");		break;
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

// #
// #
// #		Divelogs DE  Web Service Implementation.
// #
// #

struct DiveListResult
{
	QString errorCondition;
	QString errorDetails;
	QByteArray idList; // comma-separated, suitable to be sent in the fetch request
	int idCount;
};

static DiveListResult parseDiveLogsDeDiveList(const QByteArray &xmlData)
{
	/* XML format seems to be:
	 * <DiveDateReader version="1.0">
	 *   <DiveDates>
	 *     <date diveLogsId="nnn" lastModified="YYYY-MM-DD hh:mm:ss">DD.MM.YYYY hh:mm</date>
	 *     [repeat <date></date>]
	 *   </DiveDates>
	 * </DiveDateReader>
	 */
	QXmlStreamReader reader(xmlData);
	const QString invalidXmlError = DivelogsDeWebServices::tr("Invalid response from server");
	bool seenDiveDates = false;
	DiveListResult result;
	result.idCount = 0;

	if (reader.readNextStartElement() && reader.name() != "DiveDateReader") {
		result.errorCondition = invalidXmlError;
		result.errorDetails =
				DivelogsDeWebServices::tr("Expected XML tag 'DiveDateReader', got instead '%1")
				.arg(reader.name().toString());
		goto out;
	}

	while (reader.readNextStartElement()) {
		if (reader.name() != "DiveDates") {
			if (reader.name() == "Login") {
				QString status = reader.readElementText();
				// qDebug() << "Login status:" << status;

				// Note: there has to be a better way to determine a successful login...
				if (status == "failed") {
					result.errorCondition = "Login failed";
					goto out;
				}
			} else {
				// qDebug() << "Skipping" << reader.name();
			}
			continue;
		}

		// process <DiveDates>
		seenDiveDates = true;
		while (reader.readNextStartElement()) {
			if (reader.name() != "date") {
				// qDebug() << "Skipping" << reader.name();
				continue;
			}
			QStringRef id = reader.attributes().value("divelogsId");
			// qDebug() << "Found" << reader.name() << "with id =" << id;
			if (!id.isEmpty()) {
				result.idList += id.toLatin1();
				result.idList += ',';
				++result.idCount;
			}

			reader.skipCurrentElement();
		}
	}

	// chop the ending comma, if any
	result.idList.chop(1);

	if (!seenDiveDates) {
		result.errorCondition = invalidXmlError;
		result.errorDetails = DivelogsDeWebServices::tr("Expected XML tag 'DiveDates' not found");
	}

out:
	if (reader.hasError()) {
		// if there was an XML error, overwrite the result or other error conditions
		result.errorCondition = invalidXmlError;
		result.errorDetails = DivelogsDeWebServices::tr("Malformed XML response. Line %1: %2")
				.arg(reader.lineNumber()).arg(reader.errorString());
	}
	return result;
}

DivelogsDeWebServices* DivelogsDeWebServices::instance()
{
	static DivelogsDeWebServices *self = new DivelogsDeWebServices(mainWindow());
	self->setAttribute(Qt::WA_QuitOnClose, false);
	return self;
}

void DivelogsDeWebServices::downloadDives()
{
	hideUpload();
	exec();
}

void DivelogsDeWebServices::prepareDivesForUpload()
{
	QString errorText(tr("Cannot create DLD file"));
	char *filename = prepare_dives_for_divelogs(true);
	if (filename) {
		QFile f(filename);
		if (f.exists()) {
			f.open(QIODevice::ReadOnly);
			uploadDives((QIODevice *)&f);
			f.close();
			f.remove();
			return;
		}
		mainWindow()->showError(errorText.append(": ").append(filename));
		return;
	}
	mainWindow()->showError(errorText.append("!"));
}

void DivelogsDeWebServices::uploadDives(QIODevice *dldContent)
{
	QHttpMultiPart mp(QHttpMultiPart::FormDataType);
	QHttpPart part;
	QFile *f = (QFile *)dldContent;
	QFileInfo fi(*f);
	QString args("form-data; name=\"userfile\"; filename=\"" + fi.absoluteFilePath() + "\"");
	part.setRawHeader("Content-Disposition", args.toLatin1());
	part.setBodyDevice(dldContent);
	mp.append(part);

	multipart = &mp;
	hideDownload();
	exec();
	multipart = NULL;

	delete reply; // we need to ensure it has stopped using our QHttpMultiPart
}

DivelogsDeWebServices::DivelogsDeWebServices(QWidget* parent, Qt::WindowFlags f): WebServices(parent, f)
{
	QSettings s;
	ui.userID->setText(s.value("divelogde_user").toString());
	ui.password->setText(s.value("divelogde_pass").toString());
	hideUpload();
}

void DivelogsDeWebServices::startUpload()
{
	QSettings s;
	s.setValue("divelogde_user", ui.userID->text());
	s.setValue("divelogde_pass", ui.password->text());
	s.sync();

	ui.status->setText(tr("Uploading dive list..."));
	ui.progressBar->setRange(0,0); // this makes the progressbar do an 'infinite spin'
	ui.upload->setEnabled(false);
	ui.userID->setEnabled(false);
	ui.password->setEnabled(false);

	QNetworkRequest request;
	request.setUrl(QUrl("https://divelogs.de/DivelogsDirectImport.php"));
	request.setRawHeader("Accept", "text/xml, application/xml");

	QHttpPart part;
	part.setRawHeader("Content-Disposition", "form-data; name=\"user\"");
	part.setBody(ui.userID->text().toUtf8());
	multipart->append(part);

	part.setRawHeader("Content-Disposition", "form-data; name=\"pass\"");
	part.setBody(ui.password->text().toUtf8());
	multipart->append(part);

	reply = manager()->post(request, multipart);
	connect(reply, SIGNAL(finished()), this, SLOT(uploadFinished()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
		SLOT(uploadError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(uploadProgress(qint64,qint64)), this,
		SLOT(updateProgress(qint64,qint64)));

	timeout.start(30000); // 30s
}

void DivelogsDeWebServices::startDownload()
{
	ui.status->setText(tr("Downloading dive list..."));
	ui.progressBar->setRange(0,0); // this makes the progressbar do an 'infinite spin'
	ui.download->setEnabled(false);
	ui.userID->setEnabled(false);
	ui.password->setEnabled(false);

	QNetworkRequest request;
	request.setUrl(QUrl("https://divelogs.de/xml_available_dives.php"));
	request.setRawHeader("Accept", "text/xml, application/xml");
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	QUrl body;
	body.addQueryItem("user", ui.userID->text());
	body.addQueryItem("pass", ui.password->text());

	reply = manager()->post(request, body.encodedQuery());
#else
	QUrlQuery body;
	body.addQueryItem("user", ui.userID->text());
	body.addQueryItem("pass", ui.password->text());

	reply = manager()->post(request, body.query(QUrl::FullyEncoded).toLatin1())
#endif
	connect(reply, SIGNAL(finished()), this, SLOT(listDownloadFinished()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
		this, SLOT(downloadError(QNetworkReply::NetworkError)));

	timeout.start(30000); // 30s
}

void DivelogsDeWebServices::listDownloadFinished()
{
	if (!reply)
		return;
	QByteArray xmlData = reply->readAll();
	reply->deleteLater();
	reply = NULL;

	// parse the XML data we downloaded
	DiveListResult diveList = parseDiveLogsDeDiveList(xmlData);
	if (!diveList.errorCondition.isEmpty()) {
		// error condition
		resetState();
		ui.status->setText(diveList.errorCondition);
		return;
	}

	ui.status->setText(tr("Downloading %1 dives...").arg(diveList.idCount));

	QNetworkRequest request;
//	request.setUrl(QUrl("https://divelogs.de/DivelogsDirectExport.php"));
	request.setUrl(QUrl("http://divelogs.de/DivelogsDirectExport.php"));
	request.setRawHeader("Accept", "application/zip, */*");
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
	QUrl body;
	body.addQueryItem("user", ui.userID->text());
	body.addQueryItem("pass", ui.password->text());
	body.addQueryItem("ids", diveList.idList);

	reply = manager()->post(request, body.encodedQuery());
#else
	QUrlQuery body;
	body.addQueryItem("user", ui.userID->text());
	body.addQueryItem("pass", ui.password->text());
	body.addQueryItem("ids", diveList.idList);

	reply = manager()->post(request, body.query(QUrl::FullyEncoded).toLatin1())
#endif

	connect(reply, SIGNAL(readyRead()), this, SLOT(saveToZipFile()));
	connectSignalsForDownload(reply);
}

void DivelogsDeWebServices::saveToZipFile()
{
	if (!zipFile.isOpen()) {
		zipFile.setFileTemplate(QDir::tempPath() + "/import-XXXXXX.dld");
		zipFile.open();
	}

	zipFile.write(reply->readAll());
}

void DivelogsDeWebServices::downloadFinished()
{
	if (!reply)
		return;

	ui.download->setEnabled(true);
	ui.status->setText(tr("Download finished - %1").arg(reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()));
	reply->deleteLater();
	reply = NULL;

	int errorcode;
	zipFile.seek(0);
#ifdef Q_OS_UNIX
	int duppedfd = dup(zipFile.handle());
	struct zip *zip = zip_fdopen(duppedfd, 0, &errorcode);
	if (!zip)
		::close(duppedfd);
#else
	struct zip *zip = zip_open(zipFile.fileName().toLocal8Bit().data(), 0, &errorcode);
#endif
	if (!zip) {
		char buf[512];
		zip_error_to_str(buf, sizeof(buf), errorcode, errno);
		QMessageBox::critical(this, tr("Corrupted download"),
				      tr("The archive could not be opened:\n%1").arg(QString::fromLocal8Bit(buf)));
		zipFile.close();
		return;
	}
	// now allow the user to cancel or accept
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);

close_zip:
	zip_close(zip);
	zipFile.close();
}

void DivelogsDeWebServices::uploadFinished()
{
	if (!reply)
		return;

	ui.progressBar->setRange(0,1);
	ui.upload->setEnabled(true);
	ui.status->setText(tr("Upload finished"));

	// check what the server sent us: it might contain
	// an error condition, such as a failed login
	QByteArray xmlData = reply->readAll();
	char *resp = xmlData.data();
	if (resp) {
		char *parsed = strstr(resp, "<Login>");
		if (parsed) {
			if (strstr(resp, "<Login>succeeded</Login>")) {
				if (strstr(resp, "<FileCopy>failed</FileCopy>")) {
					ui.status->setText(tr("Upload failed"));
					return;
				}
				ui.status->setText(tr("Upload successful"));
				return;
			}
			ui.status->setText(tr("Login failed"));
			return;
		}
		ui.status->setText(tr("Cannot parse response"));
	}
}

void DivelogsDeWebServices::setStatusText(int status)
{

}

void DivelogsDeWebServices::downloadError(QNetworkReply::NetworkError)
{
	resetState();
	ui.status->setText(tr("Download error: %1").arg(reply->errorString()));
	reply->deleteLater();
	reply = NULL;
}

void DivelogsDeWebServices::uploadError(QNetworkReply::NetworkError error)
{
	downloadError(error);
}

void DivelogsDeWebServices::buttonClicked(QAbstractButton* button)
{
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	switch(ui.buttonBox->buttonRole(button)){
	case QDialogButtonBox::ApplyRole:{
		/* parse file and import dives */
		char *error = NULL;
		parse_file(zipFile.fileName().toLocal8Bit().data(), &error);
		if (error != NULL) {
			mainWindow()->showError(error);
			free(error);
		}
		process_dives(TRUE, FALSE);
		mainWindow()->refreshDisplay();

		/* store last entered user/pass in config */
		QSettings s;
		s.setValue("divelogde_user", ui.userID->text());
		s.setValue("divelogde_pass", ui.password->text());
		s.sync();
		hide();
		close();
		resetState();
	}
		break;
	case QDialogButtonBox::RejectRole:
		// these two seem to be causing a crash:
		// reply->deleteLater();
		// reply = NULL;
		resetState();
		break;
	case QDialogButtonBox::HelpRole:
		QDesktopServices::openUrl(QUrl("http://divelogs.de"));
		break;
	default:
		break;
	}
}
