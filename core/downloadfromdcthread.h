#ifndef DOWNLOADFROMDCTHREAD_H
#define DOWNLOADFROMDCTHREAD_H

#include <QThread>
#include "dive.h"
#include "libdivecomputer.h"

class DownloadThread : public QThread {
	Q_OBJECT
public:
	DownloadThread(QObject *parent, device_data_t *data);
	void setDiveTable(struct dive_table *table);
	void run() override;

	QString error;

private:
	device_data_t *data;
};

#endif
