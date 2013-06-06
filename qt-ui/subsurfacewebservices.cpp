#include "subsurfacewebservices.h"
#include "ui_subsurfacewebservices.h"
#include "../webservice.h"

#include <libxml/parser.h>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDebug>
#include <qdesktopservices.h>

SubsurfaceWebServices* SubsurfaceWebServices::instance()
{
	static SubsurfaceWebServices *self = new SubsurfaceWebServices();
	return self;
}

SubsurfaceWebServices::SubsurfaceWebServices(QWidget* parent, Qt::WindowFlags f)
: ui( new Ui::SubsurfaceWebServices()){
	ui->setupUi(this);
	connect(ui->buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
	connect(ui->download, SIGNAL(clicked(bool)), this, SLOT(startDownload()));
	ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
}

void SubsurfaceWebServices::buttonClicked(QAbstractButton* button)
{
	ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);
	switch(ui->buttonBox->buttonRole(button)){
		case QDialogButtonBox::ApplyRole:
			qDebug() << "Apply Clicked";
			break;
		case QDialogButtonBox::RejectRole:
			manager->deleteLater();
			reply->deleteLater();
			ui->progressBar->setMaximum(1);
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
	url.setQueryItems( QList<QPair<QString,QString> >() << qMakePair(QString("login"), ui->userID->text()));

	manager = new QNetworkAccessManager(this);
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("Accept", "text/xml");
	reply = manager->get(request);
	ui->progressBar->setRange(0,0); // this makes the progressbar do an 'infinite spin'
	ui->download->setEnabled(false);
	ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(false);

	connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), 
			this, SLOT(downloadError(QNetworkReply::NetworkError)));
}

void SubsurfaceWebServices::downloadFinished()
{
	ui->progressBar->setRange(0,1);
	downloadedData = reply->readAll();

	ui->download->setEnabled(true);
	ui->status->setText(tr("Download Finished"));

	uint resultCode = download_dialog_parse_response(downloadedData);
	setStatusText(resultCode);
	if (resultCode == DD_STATUS_OK){
		ui->buttonBox->button(QDialogButtonBox::Apply)->setEnabled(true);
	}
	manager->deleteLater();
	reply->deleteLater();
}

void SubsurfaceWebServices::downloadError(QNetworkReply::NetworkError error)
{
	ui->download->setEnabled(true);
	ui->progressBar->setRange(0,1);
	ui->status->setText(QString::number((int)QNetworkRequest::HttpStatusCodeAttribute));
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
	ui->status->setText(text);
}

void SubsurfaceWebServices::runDialog()
{
	show();
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
