// SPDX-License-Identifier: GPL-2.0
#include "uploadDiveLogsDE.h"
#include <QDir>
#include <QDebug>
#include <QHttpMultiPart>
#include <zip.h>
#include <errno.h>
#include "core/display.h"
#include "core/errorhelper.h"
#include "core/qthelper.h"
#include "core/dive.h"
#include "core/membuffer.h"
#include "core/divesite.h"
#include "core/cloudstorage.h"
#include "core/settings/qPrefCloudStorage.h"


uploadDiveLogsDE *uploadDiveLogsDE::instance()
{
	static uploadDiveLogsDE *self = new uploadDiveLogsDE;

	return self;
}


void uploadDiveLogsDE::doUpload(bool selected, const QString &userid, const QString &password)
{
	/* generate a temporary filename and create/open that file with zip_open */
	QString filename(QDir::tempPath() + "/divelogsde-upload.dld");
	qDebug() << "---> Starting upload";

	// Make zip file, with all dives, in divelogs.de format
	if (!prepareDives(selected, filename)) {
		report_error("Failed to create uploadDiveLogsDE file %s\n", qPrintable(filename));
		return;
	}
	qDebug() << "---> Prepare dives done";
	return;

	// And upload it
	QFile f(filename);
	if (f.open(QIODevice::ReadOnly)) {
		qDebug() << "---> Execute";
		uploadDives(&f, userid, password);
		qDebug() << "---> Done";
		f.close();
		f.remove();
	} else {
		report_error("Failed to open uploadDiveLogsDE file %s\n", qPrintable(filename));
	}
}


bool uploadDiveLogsDE::prepareDives(bool selected, QString filename)
{
	static const char errPrefix[] = "divelog.de-uploadDiveLogsDE:";
		xsltStylesheetPtr xslt = NULL;
	struct zip *zip;
	
	xslt = get_stylesheet("divelogs-export.xslt");
	if (!xslt) {
		qDebug() << errPrefix << "missing stylesheet";
		report_error(tr("Stylesheet to export to divelogs.de is not found").toUtf8());
		return false;
	}
	
	int error_code;
	qDebug() << "----> building zip: " << filename;
	zip = zip_open(QFile::encodeName(QDir::toNativeSeparators(filename)), ZIP_CREATE, &error_code);
	if (!zip) {
		char buffer[1024];
		zip_error_to_str(buffer, sizeof buffer, error_code, errno);
		report_error(tr("Failed to create zip file for uploadDiveLogsDE: %s").toUtf8(), buffer);
		return false;
	}
	
	/* walk the dive list in chronological order */
	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		char xmlfilename[PATH_MAX];
		int streamsize;
		const char *membuf;
		xmlDoc *transformed;
		struct zip_source *s;
		struct membuffer mb = {};
		
		/*
		 * Get the i'th dive in XML format so we can process it.
		 * We need to save to a file before we can reload it back into memory...
		 */
		if (selected && !dive->selected)
			continue;
		/* make sure the buffer is empty and add the dive */
		struct dive_site *ds = dive->dive_site;
		mb.len = 0;
		if (ds) {
			put_format(&mb, "<divelog><divesites><site uuid='%8x' name='", dive->dive_site->uuid);
			put_quoted(&mb, ds->name, 1, 0);
			put_format(&mb, "'");
			put_location(&mb, &ds->location, " gps='", "'");
			put_format(&mb, ">\n");
			if (ds->taxonomy.nr) {
				for (int j = 0; j < ds->taxonomy.nr; j++) {
					struct taxonomy *t = &ds->taxonomy.category[j];
					if (t->category != TC_NONE && t->category == prefs.geocoding.category[j] && t->value) {
						put_format(&mb, "  <geo cat='%d'", t->category);
						put_format(&mb, " origin='%d' value='", t->origin);
						put_quoted(&mb, t->value, 1, 0);
						put_format(&mb, "'/>\n");
					}
				}
			}
			put_format(&mb, "</site>\n</divesites>\n");
		}
		save_one_dive_to_mb(&mb, dive, false);

		if (ds) {
			put_format(&mb, "</divelog>\n");
		}
		membuf = mb_cstring(&mb);
		streamsize = strlen(membuf);
		/*
		 * Parse the memory buffer into XML document and
		 * transform it to divelogs.de format, finally dumping
		 * the XML into a character buffer.
		 */
		xmlDoc *doc = xmlReadMemory(membuf, streamsize, "divelog", NULL, 0);
		if (!doc) {
			qWarning() << errPrefix << "could not parse back into memory the XML file we've just created!";
			report_error(tr("internal error").toUtf8());
			zip_close(zip);
			QFile::remove(filename);
			xsltFreeStylesheet(xslt);
			return false;
		}
		free_buffer(&mb);
		
		transformed = xsltApplyStylesheet(xslt, doc, NULL);
		if (!transformed) {
			qWarning() << errPrefix << "XSLT transform failed for dive: " << i;
			report_error(tr("Conversion of dive %1 to divelogs.de format failed").arg(i).toUtf8());
			continue;
		}
		xmlDocDumpMemory(transformed, (xmlChar **)&membuf, &streamsize);
		xmlFreeDoc(doc);
		xmlFreeDoc(transformed);
		
		/*
		 * Save the XML document into a zip file.
		 */
		snprintf(xmlfilename, PATH_MAX, "%d.xml", i + 1);
		s = zip_source_buffer(zip, membuf, streamsize, 1);
		if (s) {
			int64_t ret = zip_add(zip, xmlfilename, s);
			if (ret == -1)
				qDebug() << errPrefix << "failed to include dive:" << i;
		}
	}
	xsltFreeStylesheet(xslt);
	if (zip_close(zip)) {
		int ze, se;
#if LIBZIP_VERSION_MAJOR >= 1
		zip_error_t *error = zip_get_error(zip);
		ze = zip_error_code_zip(error);
		se = zip_error_code_system(error);
#else
		zip_error_get(zip, &ze, &se);
#endif
		report_error(qPrintable(tr("error writing zip file: %s zip error %d system error %d - %s")),
					 qPrintable(QDir::toNativeSeparators(filename)), ze, se, zip_strerror(zip));
		return false;
	}
	return true;
}


void uploadDiveLogsDE::uploadDives(QFile *dldContent, const QString &userid, const QString &password)
{
	QHttpMultiPart mp(QHttpMultiPart::FormDataType);
	QHttpPart part;
	QFileInfo fi(*dldContent);

	QNetworkRequest request;
	QString args;


	// prepare header with filename (of all dives)
	args = "form-data; name=\"userfile\"; filename=\"" + fi.absoluteFilePath() + "\"";
	part.setRawHeader("Content-Disposition", args.toLatin1());
	part.setBodyDevice(dldContent);
	mp.append(part);

	// Add userid
	args = "form-data; name=\"user\"";
	part.setRawHeader("Content-Disposition", args.toLatin1());
	part.setBody(qPrefCloudStorage::divelogsde_userid().toUtf8());
	mp.append(part);

	// Add password
	args = "form-data; name=\"pass\"";
	part.setRawHeader("Content-Disposition", args.toLatin1());
	part.setBody(qPrefCloudStorage::divelogsde_password().toUtf8());
	mp.append(part);

	// Prepare network request
	request.setUrl(QUrl("https://divelogs.de/DivelogsDirectImport.php"));
	request.setRawHeader("Accept", "text/xml, application/xml");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());

	// Execute async.
	reply = manager()->post(request, &mp);

	// connect signals from upload process
	connect(reply, SIGNAL(finished()), this, SLOT(uploadFinished()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
		SLOT(uploadError(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this,
		SLOT(updateProgress(qint64, qint64)));
	connect(&timeout, SIGNAL(timeout()), this, SLOT(uploadTimeout()));

	timeout.setSingleShot(true);
	timeout.start(30000); // 30s

	if (reply != NULL && reply->isOpen()) {
		reply->abort();
		delete reply;
		reply = NULL;
	}
}


void uploadDiveLogsDE::updateProgress(qint64 current, qint64 total)
{

}


void uploadDiveLogsDE::uploadFinished()
{

}


void uploadDiveLogsDE::uploadTimeout()
{

}


void uploadDiveLogsDE::uploadError(QNetworkReply::NetworkError error)
{

}



#ifdef JAN
#include "desktop-widgets/subsurfacewebservices.h"
#include "core/webservice.h"
#include "core/settings/qPrefCloudStorage.h"
#include "desktop-widgets/mainwindow.h"
#include "desktop-widgets/usersurvey.h"
#include "commands/command.h"
#include "core/trip.h"
#include "core/file.h"
#include "desktop-widgets/mapwidget.h"
#include "desktop-widgets/tab-widgets/maintab.h"
#include "core/subsurface-string.h"

#include <QHttpMultiPart>
#include <QMessageBox>
#include <QSettings>
#include <QXmlStreamReader>
#include <qdesktopservices.h>
#include <QShortcut>



#ifdef Q_OS_UNIX
#include <unistd.h> // for dup(2)
#endif

#include <QUrlQuery>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif


WebServices::WebServices(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f), reply(0)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(ui.download, SIGNAL(clicked(bool)), this, SLOT(startDownload()));
	connect(ui.uploadDiveLogsDE, SIGNAL(clicked(bool)), this, SLOT(startuploadDiveLogsDE()));
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	defaultApplyText = ui.buttonBox->button(QDialogButtonBox::Apply)->text();
	userAgent = getUserAgent();
}

void WebServices::hidePassword()
{
	ui.password->hide();
	ui.passLabel->hide();
}

void WebServices::hideuploadDiveLogsDE()
{
	ui.uploadDiveLogsDE->hide();
	ui.download->show();
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
	ui.uploadDiveLogsDE->setEnabled(true);
	ui.userID->setEnabled(true);
	ui.password->setEnabled(true);
	ui.progressBar->reset();
	ui.progressBar->setRange(0, 1);
	ui.status->setText(QString());
	ui.buttonBox->button(QDialogButtonBox::Apply)->setText(defaultApplyText);
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
	const QString invalidXmlError = gettextFromC::tr("Invalid response from server");
	bool seenDiveDates = false;
	DiveListResult result;
	result.idCount = 0;

	if (reader.readNextStartElement() && reader.name() != "DiveDateReader") {
		result.errorCondition = invalidXmlError;
		result.errorDetails =
			gettextFromC::tr("Expected XML tag 'DiveDateReader', got instead '%1")
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
		result.errorDetails = gettextFromC::tr("Expected XML tag 'DiveDates' not found");
	}

out:
	if (reader.hasError()) {
		// if there was an XML error, overwrite the result or other error conditions
		result.errorCondition = invalidXmlError;
		result.errorDetails = gettextFromC::tr("Malformed XML response. Line %1: %2")
						.arg(reader.lineNumber())
						.arg(reader.errorString());
	}
	return result;
}

void DivelogsDeWebServices::downloadDives()
{
	uploadDiveLogsDEMode = false;
	resetState();
	hideuploadDiveLogsDE();
	exec();
}

void DivelogsDeWebServices::prepareDivesForuploadDiveLogsDE(bool selected)
{
	/* generate a random filename and create/open that file with zip_open */
	QString filename = QDir::tempPath() + "/import-" + QString::number(qrand() % 99999999) + ".dld";
	if (!amount_selected) {
		report_error(tr("No dives were selected").toUtf8());
		return;
	}
	if (prepare_dives_for_divelogs(filename, selected)) {
		QFile f(filename);
		if (f.open(QIODevice::ReadOnly)) {
			uploadDiveLogsDEDives((QIODevice *)&f);
			f.close();
			f.remove();
			return;
		} else {
			report_error("Failed to open uploadDiveLogsDE file %s\n", qPrintable(filename));
		}
	} else {
		report_error("Failed to create uploadDiveLogsDE file %s\n", qPrintable(filename));
	}
}

DivelogsDeWebServices::DivelogsDeWebServices(QWidget *parent, Qt::WindowFlags f) : WebServices(parent, f),
	multipart(NULL),
	uploadDiveLogsDEMode(false)
{
	// should DivelogDE user and pass be stored in the prefs struct or something?
	QSettings s;
	ui.userID->setText(s.value("divelogde_user").toString());
	ui.password->setText(s.value("divelogde_pass").toString());
	ui.saveUidLocal->hide();
	hideuploadDiveLogsDE();
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

void DivelogsDeWebServices::startuploadDiveLogsDE()
{
}

void DivelogsDeWebServices::saveToZipFile()
{
	if (!zipFile.isOpen()) {
		zipFile.setFileTemplate(QDir::tempPath() + "/import-XXXXXX.dld");
		zipFile.open();
	}

	zipFile.write(reply->readAll());
}


void DivelogsDeWebServices::uploadDiveLogsDEFinished()
{
	if (!reply)
		return;

	ui.progressBar->setRange(0, 1);
	ui.uploadDiveLogsDE->setEnabled(true);
	ui.userID->setEnabled(true);
	ui.password->setEnabled(true);
	ui.buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Done"));
	ui.status->setText(tr("uploadDiveLogsDE finished"));

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
					ui.status->setText(tr("uploadDiveLogsDE failed"));
					return;
				}
				ui.status->setText(tr("uploadDiveLogsDE successful"));
				return;
			}
			ui.status->setText(tr("Login failed"));
			return;
		}
		ui.status->setText(tr("Cannot parse response"));
	}
}

void DivelogsDeWebServices::setStatusText(int)
{
}

void DivelogsDeWebServices::downloadError(QNetworkReply::NetworkError)
{
	resetState();
	ui.status->setText(tr("Error: %1").arg(reply->errorString()));
	reply->deleteLater();
	reply = NULL;
}

void DivelogsDeWebServices::uploadDiveLogsDEError(QNetworkReply::NetworkError error)
{
	downloadError(error);
}

void DivelogsDeWebServices::buttonClicked(QAbstractButton *button)
{
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	switch (ui.buttonBox->buttonRole(button)) {
	case QDialogButtonBox::ApplyRole: {
		/* in 'uploadDiveLogsDEMode' button is called 'Done' and closes the dialog */
		if (uploadDiveLogsDEMode) {
			hide();
			close();
			resetState();
			break;
		}
		/* parse file and import dives */
		struct dive_table table = { 0 };
		struct trip_table trips = { 0 };
		struct dive_site_table sites = { 0 };
		parse_file(QFile::encodeName(zipFile.fileName()), &table, &trips, &sites);
		Command::importDives(&table, &trips, &sites, IMPORT_MERGE_ALL_TRIPS, QStringLiteral("divelogs.de"));

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
#endif
