// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/diveshareexportdialog.h"
#include "ui_diveshareexportdialog.h"
#include "desktop-widgets/mainwindow.h"
#include "core/save-html.h"
#include "desktop-widgets/subsurfacewebservices.h"
#include "core/qthelper.h"
#include "core/cloudstorage.h"
#include "core/uploadDiveShare.h"
#include "core/settings/qPrefCloudStorage.h"

#include <QDesktopServices>


DiveShareExportDialog::DiveShareExportDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::DiveShareExportDialog),
	exportSelected(false)
{
	ui->setupUi(this);
	// creating this connection in the .ui file appears to fail with Qt6
	connect(ui->getUIDbutton, &QPushButton::clicked, this, &DiveShareExportDialog::UIDFromBrowser);
}

DiveShareExportDialog::~DiveShareExportDialog()
{
	delete ui;
}

void DiveShareExportDialog::UIDFromBrowser()
{
	QDesktopServices::openUrl(QUrl(DIVESHARE_BASE_URI "/secret"));
}

DiveShareExportDialog *DiveShareExportDialog::instance()
{
	static DiveShareExportDialog *self = new DiveShareExportDialog(MainWindow::instance());
	self->ui->txtResult->setHtml("");
	self->ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
	return self;
}

void DiveShareExportDialog::prepareDivesForUpload(bool selected)
{
	exportSelected = selected;
	ui->frameConfigure->setVisible(true);
	ui->frameResults->setVisible(false);

	ui->txtUID->setText(qPrefCloudStorage::diveshare_uid());
	ui->chkPrivate->setChecked(qPrefCloudStorage::diveshare_private());

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

void DiveShareExportDialog::finishedSlot(bool isOk, const QString &text, const QByteArray &html)
{
	ui->progressBar->setVisible(false);
	if (!isOk) {
		ui->buttonBox->setStandardButtons(QDialogButtonBox::Cancel);
		ui->txtResult->setText(text);
	} else {
		ui->buttonBox->setStandardButtons(QDialogButtonBox::Ok);
		ui->txtResult->setHtml(generate_html_list(html));
	}
}

void DiveShareExportDialog::doUpload()
{
	//Store current settings
	QString uid = ui->txtUID->text();
	bool noPublic = ui->chkPrivate->isChecked();
	qPrefCloudStorage::set_diveshare_uid(uid);
	qPrefCloudStorage::set_diveshare_private(noPublic);

	//Change UI into results mode
	ui->frameConfigure->setVisible(false);
	ui->frameResults->setVisible(true);
	ui->progressBar->setVisible(true);
	ui->progressBar->setRange(0, 0);

	uploadDiveShare::instance()->doUpload(exportSelected, uid, noPublic);
	connect(uploadDiveShare::instance(), SIGNAL(uploadFinish(bool, const QString &, const QByteArray &)),
			this, SLOT(finishedSlot(bool, const QString &, const QByteArray &)));

	// Not implemented in the UI, but would be nice to have
	//connect(uploadDiveLogsDE::instance(), SIGNAL(uploadProgress(qreal, qreal)),
	//		this, SLOT(updateProgress(qreal, qreal)));
	//connect(uploadDiveLogsDE::instance(), SIGNAL(uploadStatus(const QString &)),
	//		this, SLOT(uploadStatus(const QString &)));
}
