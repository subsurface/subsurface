// SPDX-License-Identifier: GPL-2.0
#include "uploadDiveLogsDE.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <errno.h>
#include "core/errorhelper.h"
#include "core/qthelper.h"
#include "core/dive.h"
#include "core/divelist.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/membuffer.h"
#include "core/range.h"
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


void uploadDiveLogsDE::doUpload(bool selected, const QString &userid, const QString &password)
{
	QString err;

	// Clean up previous buffer if any
	if (mb_json.buffer)
		free(mb_json.buffer);
	mb_json.len = 0;
	mb_json.alloc = 0;
	mb_json.buffer = NULL;

	// Make json with all dives, in divelogs.de format
	if (!prepareDives(selected)) {
		emit uploadFinish(false, tr("Cannot prepare dives, none selected?"));
		timeout.stop();
		return;
	}

	// And upload it
	uploadDives(userid, password);
}


bool uploadDiveLogsDE::prepareDives(bool selected)
{
	static const char errPrefix[] = "divelog.de-upload:";

	xsltStylesheetPtr xslt_divelogs = NULL;
	xsltStylesheetPtr xslt_json = NULL;

	emit uploadStatus(tr("building json to upload"));

	xslt_divelogs = get_stylesheet("divelogs-export.xslt");
	if (!xslt_divelogs) {
		report_info("%s missing stylesheet", errPrefix);
		report_error("%s", qPrintable(tr("Stylesheet to export to divelogs.de is not found")));
		return false;
	}

	xslt_json = get_stylesheet("xml2json.xslt");
	if (!xslt_json) {
		report_info("%s missing stylesheet", errPrefix);
		report_error("%s", qPrintable(tr("Stylesheet to export to json is not found")));
		return false;
	}

	put_string(&mb_json, "[");

	/* walk the dive list in chronological order */
	for (auto [i, dive]: enumerated_range(divelog.dives)) {
		int streamsize;
		char *membuf;
		xmlDoc *transformed;
		membuffer mb;
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
			put_quoted(&mb, ds->name.c_str(), 1, 0);
			put_format(&mb, "'");
			put_location(&mb, &ds->location, " gps='", "'");
			put_format(&mb, ">\n");
			for (int i = 0; i < 3; i++) {
				if (prefs.geocoding.category[i] == TC_NONE)
					continue;
				for (auto const &t: ds->taxonomy) {
					if (t.category == prefs.geocoding.category[i] && !t.value.empty()) {
						put_format(&mb, "  <geo cat='%d'", t.category);
						put_format(&mb, " origin='%d' value='", t.origin);
						put_quoted(&mb, t.value.c_str(), 1, 0);
						put_format(&mb, "'/>\n");
					}
				}
			}
			put_format(&mb, "</site>\n</divesites>\n");
		}

		save_one_dive_to_mb(&mb, *dive, false);

		if (ds) {
			put_format(&mb, "</divelog>\n");
		}
		/*
		 * Parse the memory buffer into XML document and
		 * transform it to divelogs.de format, then transforming the XML
		 * into JSON, finally dumping the JSON into a character buffer.
		 */
		xmlDoc *doc = xmlReadMemory(mb.buffer, mb.len, "divelog", NULL, XML_PARSE_HUGE);
		if (!doc) {
			report_info("%s could not parse back into memory the XML file we've just created!", errPrefix);
			report_error("%s", qPrintable(tr("internal error")));
			xsltFreeStylesheet(xslt_divelogs);
			xsltFreeStylesheet(xslt_json);
			free_xml_params(params);
			return false;
		}

		xml_params_add_int(params, "allcylinders", prefs.include_unused_tanks);
		transformed = xsltApplyStylesheet(xslt_divelogs, doc, xml_params_get(params));
		free_xml_params(params);
		if (!transformed) {
			report_info("%s XSLT transform failed for dive: %d", errPrefix, i);
			report_error("%s", qPrintable(tr("Conversion of dive %1 to divelogs.de format failed").arg(i)));
			continue;
		}
		xmlDocDumpMemory(transformed, (xmlChar **)&membuf, &streamsize);
		xmlFreeDoc(doc);
		xmlFreeDoc(transformed);

		doc = xmlReadMemory(membuf, streamsize, "divelogsdata", NULL, XML_PARSE_HUGE);
		free(membuf);
		if (!doc) {
			report_info("%s could not parse back into memory the XML file we've just created!", errPrefix);
			report_error("%s", qPrintable(tr("internal error")));
			xsltFreeStylesheet(xslt_divelogs);
			xsltFreeStylesheet(xslt_json);
			return false;
		}
		transformed = xsltApplyStylesheet(xslt_json, doc, NULL);
		if (!transformed) {
			report_info("%s XSLT transform failed for dive: %d", errPrefix, i);
			report_error("%s", qPrintable(tr("Conversion of dive %1 to divelogs.de format failed").arg(i)));
			continue;
		}
		xsltSaveResultToString((xmlChar **)&membuf, &streamsize, transformed, xslt_json);
		xmlFreeDoc(doc);
		xmlFreeDoc(transformed);

		/*
		 * Save the JSON into the JSON list object.
		 */
		put_bytes(&mb_json, membuf, streamsize);
		put_string(&mb_json, ",");
		free(membuf);
	}
	mb_json.len--; // remove last comma
	
	// Wrap the JSON array
	put_string(&mb_json, "]");

	xsltFreeStylesheet(xslt_divelogs);
	xsltFreeStylesheet(xslt_json);
	return true;
}


void uploadDiveLogsDE::uploadDives(const QString &userid, const QString &password)
{
	QHttpPart part1, part2;
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

	emit uploadStatus(tr("Logging in to divelogs.de"));

	// First get the token
	// Add userid
	args = "form-data; name=\"user\"";
	part1.setRawHeader("Content-Disposition", args.toLatin1());
	part1.setBody(userid.toUtf8());
	multipart->append(part1);
	// Add password
	args = "form-data; name=\"pass\"";
	part2.setRawHeader("Content-Disposition", args.toLatin1());
	part2.setBody(password.toUtf8());
	multipart->append(part2);

	request.setUrl(QUrl("https://divelogs.de/api/login"));
	request.setRawHeader("Accept", "*/*");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());

	// Execute async.
	reply = manager()->post(request, multipart);
	
	// connect signals from login process
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	connect(reply, &QNetworkReply::finished, this, &uploadDiveLogsDE::loginFinishedSlot);
	connect(reply, &QNetworkReply::errorOccurred, this, &uploadDiveLogsDE::loginErrorSlot);
	connect(&timeout, &QTimer::timeout, this, &uploadDiveLogsDE::loginTimeoutSlot);
#else

	connect(reply, SIGNAL(finished()), this, SLOT(loginFinishedSlot()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this,
		SLOT(loginErrorSlot(QNetworkReply::NetworkError)));
	connect(&timeout, SIGNAL(timeout()), this, SLOT(loginTimeoutSlot()));
#endif
	timeout.start(30000); // 30s
}

	

void uploadDiveLogsDE::loginFinishedSlot()
{
	QString err;
	QByteArray json_data(mb_json.buffer, mb_json.len);
	QString token;
	QString args;
	static QNetworkRequest request;

	if (!reply)
		return;

	timeout.stop();
	err = tr("Login successful");
	emit uploadStatus(err);

	// read the token from the reply
	QByteArray response_data = reply->readAll();
	// parse json to get token
	QJsonDocument jsonDoc = QJsonDocument::fromJson(response_data);
	if (jsonDoc.isNull() || !jsonDoc.isObject()) {
		err = tr("Invalid login response from divelogs.de");
		report_error("%s", qPrintable(err));
		emit uploadFinish(false, err);
		return;
	}
	QJsonObject jsonObj = jsonDoc.object();
	if (!jsonObj.contains("bearer_token") || !jsonObj["bearer_token"].isString()) {
		err = tr("Login failed: no token received");
		report_error("%s", qPrintable(err));
		emit uploadFinish(false, err);
		return;
	}
	token = jsonObj["bearer_token"].toString();

	// Now upload the dives
	emit uploadStatus(tr("Uploading dives"));

	// Prepare header bearer token
	args = "Bearer " + token;
	request.setRawHeader("Authorization", args.toUtf8());
	
	// Prepare network request
	request.setUrl(QUrl("https://divelogs.de/api/dives"));
	request.setRawHeader("Accept", "*/*");
	request.setRawHeader("Content-Type", "application/json");
	request.setRawHeader("User-Agent", getUserAgent().toUtf8());

	// Execute async.
	reply = manager()->post(request, json_data);

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
	return;
}

void uploadDiveLogsDE::loginTimeoutSlot()
{
	timeout.stop();
	if (reply) {
		reply->deleteLater();
		reply = NULL;
	}
	QString err(tr("divelogs.de not responding"));
	report_error("%s", qPrintable(err));
	emit uploadFinish(false, err);
}


void uploadDiveLogsDE::loginErrorSlot(QNetworkReply::NetworkError error)
{
	timeout.stop();
	if (reply) {
		reply->deleteLater();
		reply = NULL;
	}
	if (error == QNetworkReply::AuthenticationRequiredError) {
		QString err(tr("Login failed: invalid username or password"));
		report_error("%s", qPrintable(err));
		emit uploadFinish(false, err);
		return;
	}
	QString err(tr("network error %1").arg(error));
	report_error("%s", qPrintable(err));
	emit uploadFinish(false, err);
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

	timeout.stop();
	err = tr("Upload successful");
	emit uploadFinish(true, err);
	return;
}


void uploadDiveLogsDE::uploadTimeoutSlot()
{
	timeout.stop();
	if (reply) {
		reply->deleteLater();
		reply = NULL;
	}
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
	QString err(tr("network error %1").arg(error));
	report_error("%s", qPrintable(err));
	emit uploadFinish(false, err);
}
