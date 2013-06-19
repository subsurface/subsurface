#ifndef DOWNLOADFROMDIVECOMPUTER_H
#define DOWNLOADFROMDIVECOMPUTER_H

#include <QDialog>
#include <QThread>
#include <QHash>
#include <QMap>
#include "../libdivecomputer.h"

namespace Ui{
	class DownloadFromDiveComputer;
}
struct device_data_t;

class DownloadThread : public QThread{
	Q_OBJECT
public:
	explicit DownloadThread(device_data_t* data);
	virtual void run();
private:
	device_data_t *data;
};

class InterfaceThread : public QThread{
	Q_OBJECT
public:
	InterfaceThread(QObject *parent, device_data_t *data) ;
	virtual void run();

signals:
	void updateInterface(int value);
private:
	device_data_t *data;
};

class QStringListModel;
class DownloadFromDCWidget : public QDialog{
	Q_OBJECT
public:
	explicit DownloadFromDCWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
	static DownloadFromDCWidget *instance();
public slots:
	void on_ok_clicked();
	void on_cancel_clicked();
	void runDialog();
	void stoppedDownloading();
	void on_vendor_currentIndexChanged(const QString& vendor);
private:
	Ui::DownloadFromDiveComputer *ui;
	InterfaceThread *thread;
	bool downloading;

	QStringList vendorList;
	QHash<QString, QStringList> productList;
	QMap<QString, dc_descriptor_t *> descriptorLookup;
	device_data_t data;

	QStringListModel *vendorModel;
	QStringListModel *productModel;
	void fill_computer_list();
public:
	bool preferDownloaded();
};

#endif
