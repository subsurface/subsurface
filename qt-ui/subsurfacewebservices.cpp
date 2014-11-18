#include "subsurfacewebservices.h"
#include "webservice.h"
#include "mainwindow.h"
#include "usersurvey.h"
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
#include <QShortcut>

#include "dive.h"
#include "divelist.h"
#include "display.h"

#ifdef Q_OS_UNIX
#include <unistd.h> // for dup(2)
#endif

#if QT_VERSION >= QT_VERSION_CHECK(5, 0, 0)
#include <QUrlQuery>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

struct dive_table gps_location_table;
static bool merge_locations_into_dives(void);

#define SAME_GROUP 6 * 3600 // six hours
//TODO: C Code. static functions are not good if we plan to have a test for them.
static bool merge_locations_into_dives(void)
{
	int i, j, tracer=0, changed=0;
	struct dive *gpsfix, *nextgpsfix, *dive;

	sort_table(&gps_location_table);

	for_each_dive (i, dive) {
		if (!dive_has_gps_location(dive)) {
			for (j = tracer; (gpsfix = get_gps_location(j, &gps_location_table)) !=NULL; j++) {
				if (dive_within_time_range (dive, gpsfix->when, SAME_GROUP)) {
					/*
					 * If position is fixed during dive. This is the good one.
					 * Asign and mark position, and end gps_location loop
					 */
					if ((dive->when <= gpsfix->when && gpsfix->when <= dive->when + dive->duration.seconds)) {
						copy_gps_location(gpsfix,dive);
						changed++;
						tracer = j;
						break;
					} else {
						/*
						 * If it is not, check if there are more position fixes in SAME_GROUP range
						 */
						if ((nextgpsfix = get_gps_location(j+1,&gps_location_table)) &&
						    dive_within_time_range (dive, nextgpsfix->when, SAME_GROUP)) {
							/*
							 * If distance from gpsfix to end of dive is shorter than distance between
							 * gpsfix and nextgpsfix, gpsfix is the good one. Asign, mark and end loop.
							 * If not, simply fail and nextgpsfix will be evaluated in next iteration.
							 */
							if ((dive->when + dive->duration.seconds - gpsfix->when) < (nextgpsfix->when - gpsfix->when)) {
								copy_gps_location(gpsfix,dive);
								tracer = j;
								break;
							}
						/*
						 * If no more positions in range, the actual is the one. Asign, mark and end loop.
						 */
						} else {
							copy_gps_location(gpsfix,dive);
							changed++;
							tracer = j;
							break;
						}
					}
				} else {
					/* If position is out of SAME_GROUP range and in the future, mark position for
					 * next dive iteration and end the gps_location loop
					 */
					if (gpsfix->when >= dive->when + dive->duration.seconds + SAME_GROUP) {
						tracer = j;
						break;
					}
				}
			}
		}
	}
	return changed > 0;
}

//TODO: C-code.
static void clear_table(struct dive_table *table)
{
	int i;
	for (i = 0; i < table->nr; i++)
		free(table->dives[i]);
	table->nr = 0;
}

// TODO: This looks like should be ported to C code. or a big part of it.
bool DivelogsDeWebServices::prepare_dives_for_divelogs(const QString &tempfile, const bool selected)
{
	static const char errPrefix[] = "divelog.de-upload:";
	if (!amount_selected) {
		report_error(tr("no dives were selected").toUtf8());
		return false;
	}

	xsltStylesheetPtr xslt = NULL;
	struct zip *zip;

	xslt = get_stylesheet("divelogs-export.xslt");
	if (!xslt) {
		qDebug() << errPrefix << "missing stylesheet";
		return false;
	}


	int error_code;
	zip = zip_open(QFile::encodeName(QDir::toNativeSeparators(tempfile)), ZIP_CREATE, &error_code);
	if (!zip) {
		char buffer[1024];
		zip_error_to_str(buffer, sizeof buffer, error_code, errno);
		report_error(tr("failed to create zip file for upload: %s").toUtf8(), buffer);
		return false;
	}

	/* walk the dive list in chronological order */
	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		FILE *f;
		char filename[PATH_MAX];
		int streamsize;
		char *membuf;
		xmlDoc *transformed;
		struct zip_source *s;

		/*
		 * Get the i'th dive in XML format so we can process it.
		 * We need to save to a file before we can reload it back into memory...
		 */
		if (selected && !dive->selected)
			continue;
		QString innerTmpFile = tempfile;
		QString tmpSuffix = QString::number(qrand() % 9999) + ".tmp";
		innerTmpFile.replace(".dld", tmpSuffix);
		f = subsurface_fopen(QFile::encodeName(QDir::toNativeSeparators(innerTmpFile)), "w+");
		if (!f) {
			report_error(tr("cannot create temporary file: %s").toUtf8(), qt_error_string().toUtf8().data());
			goto error_close_zip;
		}
		save_dive(f, dive);
		fseek(f, 0, SEEK_END);
		streamsize = ftell(f);
		rewind(f);

		membuf = (char *)malloc(streamsize + 1);
		if (!membuf || (streamsize = fread(membuf, 1, streamsize, f)) == 0) {
			report_error(tr("internal error: %s").toUtf8(), qt_error_string().toUtf8().data());
			fclose(f);
			free((void *)membuf);
			goto error_close_zip;
		}
		membuf[streamsize] = 0;
		fclose(f);
		unlink(QFile::encodeName(QDir::toNativeSeparators(innerTmpFile)));
		/*
		 * Parse the memory buffer into XML document and
		 * transform it to divelogs.de format, finally dumping
		 * the XML into a character buffer.
		 */
		xmlDoc *doc = xmlReadMemory(membuf, streamsize, "divelog", NULL, 0);
		if (!doc) {
			qWarning() << errPrefix << "could not parse back into memory the XML file we've just created!";
			report_error(tr("internal error").toUtf8());
			free((void *)membuf);
			goto error_close_zip;
		}
		free((void *)membuf);

		transformed = xsltApplyStylesheet(xslt, doc, NULL);
		xmlDocDumpMemory(transformed, (xmlChar **)&membuf, &streamsize);
		xmlFreeDoc(doc);
		xmlFreeDoc(transformed);

		/*
		 * Save the XML document into a zip file.
		 */
		snprintf(filename, PATH_MAX, "%d.xml", i + 1);
		s = zip_source_buffer(zip, membuf, streamsize, 1);
		if (s) {
			int64_t ret = zip_add(zip, filename, s);
			if (ret == -1)
				qDebug() << errPrefix << "failed to include dive:" << i;
		}
	}
	zip_close(zip);
	xsltFreeStylesheet(xslt);
	return true;

error_close_zip:
	zip_close(zip);
	QFile::remove(tempfile);
	xsltFreeStylesheet(xslt);
	return false;
}

WebServices::WebServices(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f), reply(0)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(ui.download, SIGNAL(clicked(bool)), this, SLOT(startDownload()));
	connect(ui.upload, SIGNAL(clicked(bool)), this, SLOT(startUpload()));
	connect(&timeout, SIGNAL(timeout()), this, SLOT(downloadTimedOut()));
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	timeout.setSingleShot(true);
	defaultApplyText = ui.buttonBox->button(QDialogButtonBox::Apply)->text();
	userAgent = UserSurvey::getUserAgent();
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
	ui.status->setText(tr("Transferring data..."));

	// reset the timer: 30 seconds after we last got any data
	timeout.start();
}

void WebServices::connectSignalsForDownload(QNetworkReply *reply)
{
	connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),
		this, SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(downloadProgress(qint64, qint64)), this,
		SLOT(updateProgress(qint64, qint64)));

	timeout.start(30000); // 30s
}

void WebServices::resetState()
{
	ui.download->setEnabled(true);
	ui.upload->setEnabled(true);
	ui.userID->setEnabled(true);
	ui.password->setEnabled(true);
	ui.progressBar->reset();
	ui.progressBar->setRange(0, 1);
	ui.status->setText(QString());
	ui.buttonBox->button(QDialogButtonBox::Apply)->setText(defaultApplyText);
}

// #
// #
// #		Subsurface Web Service Implementation.
// #
// #

SubsurfaceWebServices::SubsurfaceWebServices(QWidget *parent, Qt::WindowFlags f) : WebServices(parent, f)
{
	QSettings s;
	if (!prefs.save_userid_local || !*prefs.userid)
		ui.userID->setText(s.value("subsurface_webservice_uid").toString().toUpper());
	else
		ui.userID->setText(prefs.userid);
	hidePassword();
	hideUpload();
	ui.progressBar->setFormat(tr("Enter User ID and click Download"));
	ui.progressBar->setRange(0, 1);
	ui.progressBar->setValue(-1);
	ui.progressBar->setAlignment(Qt::AlignCenter);
	ui.saveUidLocal->setChecked(prefs.save_userid_local);
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

void SubsurfaceWebServices::buttonClicked(QAbstractButton *button)
{
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	switch (ui.buttonBox->buttonRole(button)) {
	case QDialogButtonBox::ApplyRole: {
		clear_table(&gps_location_table);
		QByteArray url = tr("Webservice").toLocal8Bit();
		parse_xml_buffer(url.data(), downloadedData.data(), downloadedData.length(), &gps_location_table, NULL);

		/* now merge the data in the gps_location table into the dive_table */
		if (merge_locations_into_dives()) {
			mark_divelist_changed(true);
			MainWindow::instance()->globe()->repopulateLabels();
			MainWindow::instance()->globe()->centerOnCurrentDive();
			MainWindow::instance()->information()->updateDiveInfo();
		}

		/* store last entered uid in config */
		QSettings s;
		QString qDialogUid = ui.userID->text().toUpper();
		bool qSaveUid = ui.saveUidLocal->checkState();
		set_save_userid_local(qSaveUid);
		if (qSaveUid) {
			QString qSettingUid = s.value("subsurface_webservice_uid").toString();
			QString qFileUid = QString(prefs.userid);
			bool s_eq_d = (qSettingUid == qDialogUid);
			bool d_eq_f = (qDialogUid == qFileUid);
			if (!d_eq_f || s_eq_d)
				s.setValue("subsurface_webservice_uid", qDialogUid);
			set_userid(qDialogUid.toLocal8Bit().data());
		} else {
			s.setValue("subsurface_webservice_uid", qDialogUid);
		}
		s.sync();
		hide();
		close();
		resetState();
	} break;
	case QDialogButtonBox::RejectRole:
		if (reply != NULL && reply->isOpen()) {
			reply->abort();
			delete reply;
			reply = NULL;
		}
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
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	url.addQueryItem("login", ui.userID->text().toUpper());
#else
	QUrlQuery query;
	query.addQueryItem("login", ui.userID->text().toUpper());
	url.setQuery(query);
#endif

	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("Accept", "text/xml");
	request.setRawHeader("User-Agent", userAgent.toUtf8());
	reply = manager()->get(request);
	ui.status->setText(tr("Connecting..."));
	ui.progressBar->setEnabled(true);
	ui.progressBar->setRange(0, 0); // this makes the progressbar do an 'infinite spin'
	ui.download->setEnabled(false);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	connectSignalsForDownload(reply);
}

void SubsurfaceWebServices::downloadFinished()
{
	if (!reply)
		return;

	ui.progressBar->setRange(0, 1);
	ui.progressBar->setValue(1);
	ui.progressBar->setFormat("%p%");
	downloadedData = reply->readAll();

	ui.download->setEnabled(true);
	ui.status->setText(tr("Download finished"));

	uint resultCode = download_dialog_parse_response(downloadedData);
	setStatusText(resultCode);
	if (resultCode == DD_STATUS_OK) {
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
	switch (status) {
	case DD_STATUS_ERROR_CONNECT:
		text = tr("Connection error: ");
		break;
	case DD_STATUS_ERROR_ID:
		text = tr("Invalid user identifier!");
		break;
	case DD_STATUS_ERROR_PARSE:
		text = tr("Cannot parse response!");
		break;
	case DD_STATUS_OK:
		text = tr("Download successful");
		break;
	}
	ui.status->setText(text);
}

//TODO: C-Code.
/* requires that there is a <download> or <error> tag under the <root> tag */
void SubsurfaceWebServices::download_dialog_traverse_xml(xmlNodePtr node, unsigned int *download_status)
{
	xmlNodePtr cur_node;
	for (cur_node = node; cur_node; cur_node = cur_node->next) {
		if ((!strcmp((const char *)cur_node->name, (const char *)"download")) &&
		    (!strcmp((const char *)xmlNodeGetContent(cur_node), (const char *)"ok"))) {
			*download_status = DD_STATUS_OK;
			return;
		} else if (!strcmp((const char *)cur_node->name, (const char *)"error")) {
			*download_status = DD_STATUS_ERROR_ID;
			return;
		}
	}
}

// TODO: C-Code
unsigned int SubsurfaceWebServices::download_dialog_parse_response(const QByteArray &xml)
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

struct DiveListResult {
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
	const QString invalidXmlError = QObject::tr("Invalid response from server");
	bool seenDiveDates = false;
	DiveListResult result;
	result.idCount = 0;

	if (reader.readNextStartElement() && reader.name() != "DiveDateReader") {
		result.errorCondition = invalidXmlError;
		result.errorDetails =
			QObject::tr("Expected XML tag 'DiveDateReader', got instead '%1")
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
		result.errorDetails = QObject::tr("Expected XML tag 'DiveDates' not found");
	}

out:
	if (reader.hasError()) {
		// if there was an XML error, overwrite the result or other error conditions
		result.errorCondition = invalidXmlError;
		result.errorDetails = QObject::tr("Malformed XML response. Line %1: %2")
						.arg(reader.lineNumber())
						.arg(reader.errorString());
	}
	return result;
}

DivelogsDeWebServices *DivelogsDeWebServices::instance()
{
	static DivelogsDeWebServices *self = new DivelogsDeWebServices(MainWindow::instance());
	self->setAttribute(Qt::WA_QuitOnClose, false);
	return self;
}

void DivelogsDeWebServices::downloadDives()
{
	uploadMode = false;
	resetState();
	hideUpload();
	exec();
}

void DivelogsDeWebServices::prepareDivesForUpload(bool selected)
{
	/* generate a random filename and create/open that file with zip_open */
	QString filename = QDir::tempPath() + "/import-" + QString::number(qrand() % 99999999) + ".dld";
	if (prepare_dives_for_divelogs(filename, selected)) {
		QFile f(filename);
		if (f.open(QIODevice::ReadOnly)) {
			uploadDives((QIODevice *)&f);
			f.close();
			f.remove();
			return;
		}
	}
	MainWindow::instance()->showError(get_error_string());
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
	resetState();
	uploadMode = true;
	ui.buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(true);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Done"));
	exec();

	multipart = NULL;
	if (reply != NULL && reply->isOpen()) {
		reply->abort();
		delete reply;
		reply = NULL;
	}
}

DivelogsDeWebServices::DivelogsDeWebServices(QWidget *parent, Qt::WindowFlags f) : WebServices(parent, f), uploadMode(false)
{
	QSettings s;
	ui.userID->setText(s.value("divelogde_user").toString());
	ui.password->setText(s.value("divelogde_pass").toString());
	ui.saveUidLocal->hide();
	hideUpload();
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

void DivelogsDeWebServices::startUpload()
{
	QSettings s;
	s.setValue("divelogde_user", ui.userID->text());
	s.setValue("divelogde_pass", ui.password->text());
	s.sync();

	ui.status->setText(tr("Uploading dive list..."));
	ui.progressBar->setRange(0, 0); // this makes the progressbar do an 'infinite spin'
	ui.upload->setEnabled(false);
	ui.userID->setEnabled(false);
	ui.password->setEnabled(false);

	QNetworkRequest request;
	request.setUrl(QUrl("https://divelogs.de/DivelogsDirectImport.php"));
	request.setRawHeader("Accept", "text/xml, application/xml");
	request.setRawHeader("User-Agent", userAgent.toUtf8());

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
	connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this,
		SLOT(updateProgress(qint64, qint64)));

	timeout.start(30000); // 30s
}

void DivelogsDeWebServices::startDownload()
{
	ui.status->setText(tr("Downloading dive list..."));
	ui.progressBar->setRange(0, 0); // this makes the progressbar do an 'infinite spin'
	ui.download->setEnabled(false);
	ui.userID->setEnabled(false);
	ui.password->setEnabled(false);

	QNetworkRequest request;
	request.setUrl(QUrl("https://divelogs.de/xml_available_dives.php"));
	request.setRawHeader("Accept", "text/xml, application/xml");
	request.setRawHeader("User-Agent", userAgent.toUtf8());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
	QUrl body;
	body.addQueryItem("user", ui.userID->text());
	body.addQueryItem("pass", ui.password->text());

	reply = manager()->post(request, body.encodedQuery());
#else
	QUrlQuery body;
	body.addQueryItem("user", ui.userID->text());
	body.addQueryItem("pass", ui.password->text());

	reply = manager()->post(request, body.query(QUrl::FullyEncoded).toLatin1());
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
	request.setUrl(QUrl("https://divelogs.de/DivelogsDirectExport.php"));
	request.setRawHeader("Accept", "application/zip, */*");
	request.setRawHeader("User-Agent", userAgent.toUtf8());
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
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

	reply = manager()->post(request, body.query(QUrl::FullyEncoded).toLatin1());
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
#if defined(Q_OS_UNIX) && defined(LIBZIP_VERSION_MAJOR)
	int duppedfd = dup(zipFile.handle());
	struct zip *zip = zip_fdopen(duppedfd, 0, &errorcode);
	if (!zip)
		::close(duppedfd);
#else
	struct zip *zip = zip_open(QFile::encodeName(zipFile.fileName()), 0, &errorcode);
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

	zip_close(zip);
	zipFile.close();
#if defined(Q_OS_UNIX) && defined(LIBZIP_VERSION_MAJOR)
	::close(duppedfd);
#endif
}

void DivelogsDeWebServices::uploadFinished()
{
	if (!reply)
		return;

	ui.progressBar->setRange(0, 1);
	ui.upload->setEnabled(true);
	ui.userID->setEnabled(true);
	ui.password->setEnabled(true);
	ui.buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Done"));
	ui.status->setText(tr("Upload finished"));

	// check what the server sent us: it might contain
	// an error condition, such as a failed login
	QByteArray xmlData = reply->readAll();
	reply->deleteLater();
	reply = NULL;
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
	ui.status->setText(tr("Error: %1").arg(reply->errorString()));
	reply->deleteLater();
	reply = NULL;
}

void DivelogsDeWebServices::uploadError(QNetworkReply::NetworkError error)
{
	downloadError(error);
}

void DivelogsDeWebServices::buttonClicked(QAbstractButton *button)
{
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	switch (ui.buttonBox->buttonRole(button)) {
	case QDialogButtonBox::ApplyRole: {
		/* in 'uploadMode' button is called 'Done' and closes the dialog */
		if (uploadMode) {
			hide();
			close();
			resetState();
			break;
		}
		/* parse file and import dives */
		parse_file(QFile::encodeName(zipFile.fileName()));
		process_dives(true, false);
		MainWindow::instance()->refreshDisplay();

		/* store last entered user/pass in config */
		QSettings s;
		s.setValue("divelogde_user", ui.userID->text());
		s.setValue("divelogde_pass", ui.password->text());
		s.sync();
		hide();
		close();
		resetState();
	} break;
	case QDialogButtonBox::RejectRole:
		// these two seem to be causing a crash:
		// reply->deleteLater();
		resetState();
		break;
	case QDialogButtonBox::HelpRole:
		QDesktopServices::openUrl(QUrl("http://divelogs.de"));
		break;
	default:
		break;
	}
}

UserSurveyServices::UserSurveyServices(QWidget *parent, Qt::WindowFlags f) : WebServices(parent, f)
{

}

QNetworkReply* UserSurveyServices::sendSurvey(QString values)
{
	QNetworkRequest request;
	request.setUrl(QString("http://subsurface-divelog.org/survey?%1").arg(values));
	request.setRawHeader("Accept", "text/xml");
	request.setRawHeader("User-Agent", userAgent.toUtf8());
	reply = manager()->get(request);
	return reply;
}
