#include "downloadfromdivecomputer.h"
#include "ui_downloadfromdivecomputer.h"

#include "../libdivecomputer.h"

#include <QThread>
#include <QDebug>

namespace DownloadFromDcGlobal{
	const char *err_string;
};

extern const char *progress_bar_text;
extern double progress_bar_fraction;

DownloadFromDCWidget::DownloadFromDCWidget(QWidget* parent, Qt::WindowFlags f) :
	QDialog(parent, f), ui(new Ui::DownloadFromDiveComputer), thread(0)
{
	ui->setupUi(this);
	ui->progressBar->hide();
	ui->progressBar->setMinimum(0);
	ui->progressBar->setMaximum(100);
}

void DownloadFromDCWidget::on_cancel_clicked()
{
	close();
}

void DownloadFromDCWidget::on_ok_clicked()
{

	ui->progressBar->setValue(0);
	ui->progressBar->show();

	if(thread){
		thread->deleteLater();
	}

	device_data_t data;
	// still need to fill the data info here.
	thread = new InterfaceThread(this, &data);
	connect(thread, SIGNAL(updateInterface(int)), ui->progressBar, SLOT(setValue(int)), Qt::QueuedConnection); // Qt::QueuedConnection == threadsafe.
	connect(thread, SIGNAL(updateInterface(int)), this, SLOT(setValue(int)), Qt::QueuedConnection); // Qt::QueuedConnection == threadsafe.
	thread->start();
}

DownloadThread::DownloadThread(device_data_t* data): data(data)
{
}

void DownloadThread::run()
{
	do_libdivecomputer_import(data);
	qDebug() << "Download thread started";
}

InterfaceThread::InterfaceThread(QObject* parent, device_data_t* data): QThread(parent), data(data)
{

}

void InterfaceThread::run()
{
	DownloadThread *download = new DownloadThread(data);

	download->start();
 	while(download->isRunning()){
		msleep(200);
 		updateInterface(progress_bar_fraction *100);
 	}
	updateInterface(100);
}
