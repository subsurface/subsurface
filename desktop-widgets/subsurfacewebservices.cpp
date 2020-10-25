// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/subsurfacewebservices.h"
#include "core/qthelper.h"
#include "core/webservice.h"
#include "core/settings/qPrefCloudStorage.h"
#include "desktop-widgets/mainwindow.h"
#include "commands/command.h"
#include "core/device.h"
#include "core/divesite.h"
#include "core/trip.h"
#include "core/errorhelper.h"
#include "core/file.h"
#include "desktop-widgets/mapwidget.h"
#include "desktop-widgets/tab-widgets/maintab.h"
#include "core/selection.h"
#include "core/membuffer.h"
#include "core/cloudstorage.h"
#include "core/subsurface-string.h"
#include "core/uploadDiveLogsDE.h"
#include "core/settings/qPrefCloudStorage.h"

#include <QDir>
#include <QHttpMultiPart>
#include <QMessageBox>
#include <QXmlStreamReader>
#include <qdesktopservices.h>
#include <QShortcut>
#include <QDebug>
#include <errno.h>
#include <zip.h>

#ifdef Q_OS_UNIX
#include <unistd.h> // for dup(2)
#endif

#include <QUrlQuery>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

WebServices::WebServices(QWidget *parent) : QDialog(parent, QFlag(0)), reply(0)
{
	ui.setupUi(this);
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));
	connect(ui.download, SIGNAL(clicked(bool)), this, SLOT(startDownload()));
	connect(ui.upload, SIGNAL(clicked(bool)), this, SLOT(startUpload()));
	connect(&timeout, SIGNAL(timeout()), this, SLOT(downloadTimedOut()));
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	timeout.setSingleShot(true);
	defaultApplyText = ui.buttonBox->button(QDialogButtonBox::Apply)->text();
	userAgent = getUserAgent();
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

DivelogsDeWebServices *DivelogsDeWebServices::instance()
{
	static DivelogsDeWebServices *self = new DivelogsDeWebServices(MainWindow::instance());
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
	// this is called when the user selects the divelogs.de radiobutton

	// Remember if all dives or selected dives are to be uploaded
	useSelectedDives = selected;

	// Adjust UI
	hideDownload();
	resetState();
	uploadMode = true;
	ui.buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(true);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Done"));
	exec();
}

DivelogsDeWebServices::DivelogsDeWebServices(QWidget *parent) : WebServices(parent),
	uploadMode(false)
{
	// should DivelogDE user and pass be stored in the prefs struct or something?
	ui.userID->setText(qPrefCloudStorage::divelogde_user());
	ui.password->setText(qPrefCloudStorage::divelogde_pass());
	ui.saveUidLocal->hide();
	hideUpload();
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
}

void DivelogsDeWebServices::startUpload()
{
	qPrefCloudStorage::set_divelogde_user(ui.userID->text());
	qPrefCloudStorage::set_divelogde_pass(ui.password->text());

	ui.status->setText(tr("Uploading dive list..."));
	ui.progressBar->setRange(0, 0); // this makes the progressbar do an 'infinite spin'
	ui.upload->setEnabled(false);
	ui.userID->setEnabled(false);
	ui.password->setEnabled(false);

	// do upload in shared backend
	connect(uploadDiveLogsDE::instance(), SIGNAL(uploadFinish(bool, const QString &)),
			this, SLOT(uploadFinished(bool, const QString &)));
	connect(uploadDiveLogsDE::instance(), SIGNAL(uploadProgress(qreal, qreal)),
			this, SLOT(updateProgress(qreal, qreal)));
	connect(uploadDiveLogsDE::instance(), SIGNAL(uploadStatus(const QString &)),
			this, SLOT(uploadStatus(const QString &)));
	uploadDiveLogsDE::instance()->doUpload(useSelectedDives,
							   qPrefCloudStorage::divelogde_user(),
							   qPrefCloudStorage::divelogde_pass());
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

	QUrlQuery body;
	body.addQueryItem("user", ui.userID->text());
	body.addQueryItem("pass", ui.password->text().replace("+", "%2b"));

	reply = manager()->post(request, body.query(QUrl::FullyEncoded).toLatin1());
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

	QUrlQuery body;
	body.addQueryItem("user", ui.userID->text());
	body.addQueryItem("pass", ui.password->text().replace("+", "%2b"));
	body.addQueryItem("ids", diveList.idList);

	reply = manager()->post(request, body.query(QUrl::FullyEncoded).toLatin1());
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
	struct zip *zip = NULL;
	if (duppedfd >= 0) {
		zip = zip_fdopen(duppedfd, 0, &errorcode);
		if (!zip)
			::close(duppedfd);
	} else {
		QMessageBox::critical(this, tr("Problem with download"),
				      tr("The archive could not be opened:\n%1").arg(QString::fromLocal8Bit(strerror(errno))));
		return;
	}
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

void DivelogsDeWebServices::uploadFinished(bool success, const QString &text)
{
	ui.progressBar->setRange(0, 1);
	ui.upload->setEnabled(true);
	ui.userID->setEnabled(true);
	ui.password->setEnabled(true);
	ui.buttonBox->button(QDialogButtonBox::Cancel)->setEnabled(false);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
	ui.buttonBox->button(QDialogButtonBox::Apply)->setText(tr("Done"));
	ui.status->setText(text);
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

void DivelogsDeWebServices::updateProgress(qreal current, qreal total)
{
	ui.progressBar->setRange(0, lrint(total));
	ui.progressBar->setValue(lrint(current));
	ui.status->setText(tr("Transferring data..."));
}

void DivelogsDeWebServices::uploadStatus(const QString &text)
{
	ui.status->setText(text);
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
		struct dive_table table = empty_dive_table;
		struct trip_table trips = empty_trip_table;
		struct dive_site_table sites = empty_dive_site_table;
		struct device_table devices;
		struct filter_preset_table filter_presets;
		parse_file(QFile::encodeName(zipFile.fileName()), &table, &trips, &sites, &devices, &filter_presets);
		Command::importDives(&table, &trips, &sites, &devices, nullptr, IMPORT_MERGE_ALL_TRIPS, QStringLiteral("divelogs.de"));

		/* store last entered user/pass in config */
		qPrefCloudStorage::set_divelogde_user(ui.userID->text());
		qPrefCloudStorage::set_divelogde_pass(ui.password->text());
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
