#include "diveshareexportdialog.h"
#include "ui_diveshareexportdialog.h"
#include "mainwindow.h"
#include "save-html.h"
#include "qt-ui/usersurvey.h"
#include "qt-ui/subsurfacewebservices.h"

#include <QDesktopServices>
#include <QUrl>
#include <QSettings>

DiveShareExportDialog::DiveShareExportDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DiveShareExportDialog),
	reply(NULL)
{
	ui->setupUi(this);
}

DiveShareExportDialog::~DiveShareExportDialog()
{
	delete ui;
	delete reply;
}

void DiveShareExportDialog::UIDFromBrowser()
{
	QDesktopServices::openUrl(QUrl(DIVESHARE_BASE_URI "/secret"));
}

DiveShareExportDialog *DiveShareExportDialog::instance()
{
	static DiveShareExportDialog *self = new DiveShareExportDialog(MainWindow::instance());
	self->setAttribute(Qt::WA_QuitOnClose, false);
	self->ui->txtResult->setHtml("");
	self->ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
	return self;
}

void DiveShareExportDialog::prepareDivesForUpload(bool selected)
{
	exportSelected = selected;
	ui->frameConfigure->setVisible(true);
	ui->frameResults->setVisible(false);

	QSettings settings;
	if (settings.contains("diveshareExport/uid"))
		ui->txtUID->setText(settings.value("diveshareExport/uid").toString());

	if (settings.contains("diveshareExport/private"))
		ui->chkPrivate->setChecked(settings.value("diveshareExport/private").toBool());

	show();
}

static QByteArray generate_html_list(const QByteArray &data)
{
	QList<QByteArray> dives = data.split('\n');
	QByteArray html;
	html.append("<html><body><table>");
	for (int i = 0; i < dives.length(); i++ ) {
		html.append("<tr>");
		QList<QByteArray> dive_details = dives[i].split(',');
		if (dive_details.length() < 3)
			continue;

		QByteArray dive_id = dive_details[0];
		QByteArray dive_delete = dive_details[1];

		html.append("<td>");
		html.append("<a href=\"" DIVESHARE_BASE_URI "/dive/" + dive_id + "\">");

		//Title gets separated too, this puts it back together
		const char *sep = "";
		for (int t = 2; t < dive_details.length(); t++) {
			html.append(sep);
			html.append(dive_details[t]);
			sep = ",";
		}

		html.append("</a>");
		html.append("</td>");
		html.append("<td>");
		html.append("<a href=\"" DIVESHARE_BASE_URI "/delete/dive/" + dive_delete + "\">Delete dive</a>");
		html.append("</td>"  );

		html.append("</tr>");
	}

	html.append("</table></body></html>");
	return html;
}

void DiveShareExportDialog::finishedSlot()
{
	ui->progressBar->setVisible(false);
	if (reply->error() != 0) {
		ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
		ui->txtResult->setText(reply->errorString());
	} else {
		ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);
		ui->txtResult->setHtml(generate_html_list(reply->readAll()));
	}

	reply->deleteLater();
}

void DiveShareExportDialog::doUpload()
{
	//Store current settings
	QSettings settings;
	settings.setValue("diveshareExport/uid", ui->txtUID->text());
	settings.setValue("diveshareExport/private", ui->chkPrivate->isChecked());

	//Change UI into results mode
	ui->frameConfigure->setVisible(false);
	ui->frameResults->setVisible(true);
	ui->progressBar->setVisible(true);
	ui->progressBar->setRange(0, 0);

	//generate json
	struct membuffer buf = { 0 };
	export_list(&buf, NULL, exportSelected, false);
	QByteArray json_data(buf.buffer, buf.len);
	free_buffer(&buf);

	//Request to server
	QNetworkRequest request;

	if (ui->chkPrivate->isChecked())
		request.setUrl(QUrl(DIVESHARE_BASE_URI "/upload?private=true"));
	else
		request.setUrl(QUrl(DIVESHARE_BASE_URI "/upload"));

	request.setRawHeader("User-Agent", UserSurvey::getUserAgent().toUtf8());
	if (ui->txtUID->text().length() != 0)
		request.setRawHeader("X-UID", ui->txtUID->text().toUtf8());

	reply = WebServices::manager()->put(request, json_data);

	QObject::connect(reply, SIGNAL(finished()), this, SLOT(finishedSlot()));
}
