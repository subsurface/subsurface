// SPDX-License-Identifier: GPL-2.0
#include "uploadDiveLogsDE.h"
#include <QDir>
#include <QTemporaryFile>
#include <zip.h>
#include <errno.h>
#include "core/errorhelper.h"
#include "core/qthelper.h"
#include "core/dive.h"
#include "core/membuffer.h"
#include "core/divesite.h"
#include "core/cloudstorage.h"
#include "core/xmlparams.h"
#ifndef SUBSURFACE_MOBILE
#include "core/selection.h"
#endif // SUBSURFACE_MOBILE
#include "core/settings/qPrefCloudStorage.h"


uploadDiveLogsDE *uploadDiveLogsDE::instance()
{
	static uploadDiveLogsDE *self = new uploadDiveLogsDE;

	return self;
}


uploadDiveLogsDE::uploadDiveLogsDE():
	reply(NULL),
	multipart(NULL)
{
	timeout.setSingleShot(true);
}


static QString makeTempFileName()
{
	QTemporaryFile tmpfile;
	tmpfile.setFileTemplate(QDir::tempPath() + "/divelogsde-upload.XXXXXXXX.dld");
	tmpfile.open();
	return tmpfile.fileName();
}


void uploadDiveLogsDE::doUpload(bool selected, const QString &userid, const QString &password)
{
	QString err;

	QString filename = makeTempFileName();

	// Make zip file, with all dives, in divelogs.de format
	if (!prepareDives(filename, selected)) {
		emit uploadFinish(false, tr("Cannot prepare dives, none selected?"));
		timeout.stop();
		return;
	}

	// And upload it
	uploadDives(filename, userid, password);
}


bool uploadDiveLogsDE::prepareDives(const QString &tempfile, bool selected)
{
	static const char errPrefix[] = "divelog.de-upload:";

	xsltStylesheetPtr xslt = NULL;
	struct zip *zip;

	emit uploadStatus(tr("building zip file to upload"));

	xslt = get_stylesheet("divelogs-export.xslt");
	if (!xslt) {
		report_info("%s missing stylesheet", errPrefix);
		report_error("%s", qPrintable(tr("Stylesheet to export to divelogs.de is not found")));
		return false;
	}

	// Prepare zip file
	int error_code;
	zip = zip_open(QFile::encodeName(QDir::toNativeSeparators(tempfile)), ZIP_CREATE, &error_code);
	if (!zip) {
		char buffer[1024];
		zip_error_to_str(buffer, sizeof buffer, error_code, errno);
		report_error(qPrintable(tr("Failed to create zip file for upload: %s")), buffer);
		return false;
	}

	/* walk the dive list in chronological order */
	int i;
	struct dive *dive;
	for_each_dive (i, dive) {
		char filename[PATH_MAX];
		int streamsize;
		char *membuf;
		xmlDoc *transformed;
		struct zip_source *s;
		struct membufferpp mb;
		struct xml_params *params = alloc_xml_params();

		/*
		 * Get the i'th dive in XML format so we can process it.
		 * We need to save to a file before we can reload it back into memory...
		 */
		if (selected && !dive->selected) {
			free_xml_params(params);
			continue;
		}

		/* add the dive */
		struct dive_site *ds = dive->dive_site;

		if (ds) {
			put_format(&mb, "<divelog><divesites><site uuid='%8x' name='", ds->uuid);
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
		/*
		 * Parse the memory buffer into XML document and
		 * transform it to divelogs.de format, finally dumping
		 * the XML into a character buffer.
		 */
		xmlDoc *doc = xmlReadMemory(mb.buffer, mb.len, "divelog", NULL, XML_PARSE_HUGE);
		if (!doc) {
			report_info("%s could not parse back into memory the XML file we've just created!", errPrefix);
			report_error("%s", qPrintable(tr("internal error")));
			zip_close(zip);
			QFile::remove(tempfile);
			xsltFreeStylesheet(xslt);
			free_xml_params(params);
			return false;
		}

		xml_params_add_int(params, "allcylinders", prefs.include_unused_tanks);
		transformed = xsltApplyStylesheet(xslt, doc, xml_params_get(params));
		free_xml_params(params);
		if (!transformed) {
			report_info("%s XSLT transform failed for dive: %d", errPrefix, i);
			report_error("%s", qPrintable(tr("Conversion of dive %1 to divelogs.de format failed").arg(i)));
			continue;
		}
		xmlDocDumpMemory(transformed, (xmlChar **)&membuf, &streamsize);
		xmlFreeDoc(doc);
		xmlFreeDoc(transformed);

		/*
		 * Save the XML document into a zip file.
		 */
		snprintf(filename, PATH_MAX, "%d.xml", i + 1);
		s = zip_source_buffer(zip, membuf, streamsize, 1); // frees membuffer!
		if (s) {
			int64_t ret = zip_add(zip, filename, s);
			if (ret == -1)
				report_info("%s failed to include dive: %d", errPrefix, i);
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
			     qPrintable(QDir::toNativeSeparators(tempfile)), ze, se, zip_strerror(zip));
		return false;
	}
	return true;
}


void uploadDiveLogsDE::cleanupTempFile()
{
	if (tempFile.isOpen())
		tempFile.remove();
}


void uploadDiveLogsDE::uploadDives(const QString &filename, const QString &userid, const QString &password)
{
	QHttpPart part1, part2, part3;
	static QNetworkRequest request;
	QString args;

	// Check if there is an earlier request open
	if (reply != NULL && reply->isOpen()) {
		reply->abort();
		delete reply;
		reply = NULL;
	}
	if (multipart != NULL) {
		delete multipart;
		multipart = NULL;
	}
	multipart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

	emit uploadStatus(tr("Uploading dives"));

	// prepare header with filename (of all dives) and pointer to file
	args = "form-data; name=\"userfile\"; filename=\"" + filename + "\"";
	part1.setRawHeader("Content-Disposition", args.toLatin1());

	// open new file for reading
	cleanupTempFile();
	tempFile.setFileName(filename);
	if (!tempFile.open(QIODevice::ReadOnly)) {
		report_info("ERROR opening zip file: %s", qPrintable(filename));
		return;
	}
	part1.setBodyDevice(&tempFile);
	multipart->append(part1);

	// Add userid
	args = "form-data; name=\"user\"";
	part2.setRawHeader("Content-Disposition", args.toLatin1());
	part2.setBody(qPrefCloudStorage::divelogde_user().toUtf8());
	multipart->append(part2);

	// Add password
	args = "form-data; name=\"pass\"";
	part3.setRawHeader("Content-Disposition", args.toLatin1());
	part3.setBody(qPrefCloudStorage::divelogde_pass().toUtf8());
	multipart->append(part3);

	// Prepare network request
	request.setUrl(QUrl("https://divelogs.de/DivelogsDirectImport.php"));
	request.setRawHeader("Accept", "text/xml, application/xml");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());

	// Execute async.
	reply = manager()->post(request, multipart);

	// connect signals from upload process
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	connect(reply, &QNetworkReply::finished, this, &uploadDiveLogsDE::uploadFinishedSlot);
	connect(reply, &QNetworkReply::errorOccurred, this, &uploadDiveLogsDE::uploadErrorSlot);
	connect(reply, &QNetworkReply::uploadProgress, this, &uploadDiveLogsDE::updateProgressSlot);
	connect(&timeout, &QTimer::timeout, this, &uploadDiveLogsDE::uploadTimeoutSlot);
#else

	connect(reply, SIGNAL(finished()), this, SLOT(uploadFinishedSlot()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
		SLOT(uploadErrorSlot(QNetworkReply::NetworkError)));
	connect(reply, SIGNAL(uploadProgress(qint64, qint64)), this,
		SLOT(updateProgressSlot(qint64, qint64)));
	connect(&timeout, SIGNAL(timeout()), this, SLOT(uploadTimeoutSlot()));
#endif
	timeout.start(30000); // 30s
}


void uploadDiveLogsDE::updateProgressSlot(qint64 current, qint64 total)
{
	if (!reply)
		return;

	if (total <= 0 || current <= 0)
		return;

	// Calculate percentage as 0.x (values between 0.0 and 1.0)
	// And signal whoever wants to know
	qreal percentage = (float)current / (float)total;
	emit uploadProgress(percentage, 1.0);

	// reset the timer: 30 seconds after we last got any data
	timeout.start();
}


void uploadDiveLogsDE::uploadFinishedSlot()
{
	QString err;

	if (!reply)
		return;

	// check what the server sent us: it might contain
	// an error condition, such as a failed login
	QByteArray xmlData = reply->readAll();
	reply->deleteLater();
	reply = NULL;
	cleanupTempFile();
	char *resp = xmlData.data();
	if (resp) {
		char *parsed = strstr(resp, "<Login>");
		if (parsed) {
			if (strstr(resp, "<Login>succeeded</Login>")) {
				if (strstr(resp, "<FileCopy>failed</FileCopy>")) {
					report_error("%s", qPrintable(tr("Upload failed")));
					return;
				}
				timeout.stop();
				err = tr("Upload successful");
				emit uploadFinish(true, err);
				return;
			}
			timeout.stop();
			err = tr("Login failed");
			report_error("%s", qPrintable(err));
			emit uploadFinish(false, err);
			return;
		}
		timeout.stop();
		err = tr("Cannot parse response");
		report_error("%s", qPrintable(tr("Cannot parse response")));
		emit uploadFinish(false, err);
	}
}


void uploadDiveLogsDE::uploadTimeoutSlot()
{
	timeout.stop();
	if (reply) {
		reply->deleteLater();
		reply = NULL;
	}
	cleanupTempFile();
	QString err(tr("divelogs.de not responding"));
	report_error("%s", qPrintable(err));
	emit uploadFinish(false, err);
}


void uploadDiveLogsDE::uploadErrorSlot(QNetworkReply::NetworkError error)
{
	timeout.stop();
	if (reply) {
		reply->deleteLater();
		reply = NULL;
	}
	cleanupTempFile();
	QString err(tr("network error %1").arg(error));
	report_error("%s", qPrintable(err));
	emit uploadFinish(false, err);
}
