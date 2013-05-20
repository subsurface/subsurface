#include "downloadfromdivecomputer.h"
#include "ui_downloadfromdivecomputer.h"

#include "../libdivecomputer.h"

#include <QThread>
#include <QDebug>

namespace DownloadFromDcGlobal{
	const char *err_string;
};

DownloadFromDCWidget::DownloadFromDCWidget(QWidget* parent, Qt::WindowFlags f) :
	QDialog(parent, f), ui(new Ui::DownloadFromDiveComputer), thread(0), downloading(false)
{
	ui->setupUi(this);
	ui->progressBar->hide();
	ui->progressBar->setMinimum(0);
	ui->progressBar->setMaximum(100);
}

void DownloadFromDCWidget::on_cancel_clicked()
{
	import_thread_cancelled = true;
	if(thread){
		thread->wait();
		thread->deleteLater();
		thread = 0;
	}
	close();
}

void DownloadFromDCWidget::on_ok_clicked()
{
	if (downloading)
		return;

	ui->progressBar->setValue(0);
	ui->progressBar->show();

	if(thread){
		thread->deleteLater();
	}

	device_data_t data;
	// still need to fill the data info here.
	thread = new InterfaceThread(this, &data);
	connect(thread, SIGNAL(updateInterface(int)),
			ui->progressBar, SLOT(setValue(int)), Qt::QueuedConnection); // Qt::QueuedConnection == threadsafe.

	connect(thread, SIGNAL(finished()), this, SLOT(close()));

	thread->start();
	downloading = true;
}

DownloadThread::DownloadThread(device_data_t* data): data(data)
{
}

void DownloadThread::run()
{
	do_libdivecomputer_import(data);
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
