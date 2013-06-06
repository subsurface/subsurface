#include "subsurfacewebservices.h"
#include "ui_subsurfacewebservices.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDebug>

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
}

void SubsurfaceWebServices::buttonClicked(QAbstractButton* button)
{

}

void SubsurfaceWebServices::startDownload()
{
	QUrl url("http://api.hohndel.org/api/dive/get/");
	url.setQueryItems( QList<QPair<QString,QString> >() << qMakePair(QString("login"), ui->userID->text()));
	qDebug() << url;
	
	QNetworkAccessManager *manager = new QNetworkAccessManager(this);
	QNetworkRequest request;
	request.setUrl(url);
	request.setRawHeader("Accept", "text/xml");
	reply = manager->get(request);
	ui->progressBar->setRange(0,0);
	connect(reply, SIGNAL(finished()), this, SLOT(downloadFinished()));
	connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(downloadError(QNetworkReply::NetworkError)));
}

void SubsurfaceWebServices::downloadFinished()
{
	ui->progressBar->setRange(0,1);
	QByteArray result = reply->readAll();
	qDebug() << result;
	ui->status->setText(tr("Download Finished"));
}

void SubsurfaceWebServices::downloadError(QNetworkReply::NetworkError error)
{
	ui->progressBar->setRange(0,1);
	ui->status->setText(QString::number((int)QNetworkRequest::HttpStatusCodeAttribute));
}


void SubsurfaceWebServices::runDialog()
{
	show();
}
