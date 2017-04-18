#include "downloadfromdcthread.h"
#include "core/libdivecomputer.h"

static QString str_error(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	const QString str = QString().vsprintf(fmt, args);
	va_end(args);

	return str;
}

DownloadThread::DownloadThread(QObject *parent, device_data_t *data) : QThread(parent),
	data(data)
{
	data->download_table = nullptr;
}

void DownloadThread::setDiveTable(struct dive_table* table)
{
	data->download_table = table;
}

void DownloadThread::run()
{
	Q_ASSERT(data->download_table != nullptr);
	const char *errorText;
	import_thread_cancelled = false;
	if (!strcmp(data->vendor, "Uemis"))
		errorText = do_uemis_import(data);
	else
		errorText = do_libdivecomputer_import(data);
	if (errorText)
		error = str_error(errorText, data->devname, data->vendor, data->product);
}
