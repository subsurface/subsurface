// SPDX-License-Identifier: GPL-2.0
#include "qt-models/diveimportedmodel.h"

#include <QObject>
#include <QUndoStack>


void cliDownloader(const char *vendor, const char *product, const char *device)
{
	DiveImportedModel *diveImportedModel = new DiveImportedModel();
	DiveImportedModel::connect(diveImportedModel, &DiveImportedModel::downloadFinished, [] {
		// do something useful at the end of the download
	});

	auto data = diveImportedModel->thread.data();
	data->setVendor(vendor);
	data->setProduct(product);
	if (data->vendor() == "Uemis") {
		char *colon;
		char *devname = strdup(device);
		if ((colon = strstr(devname, ":\\ (UEMISSDA)")) != NULL) {
			*(colon + 2) = '\0';
			fprintf(stderr, "shortened devname to \"%s\"", devname);
		}
		data->setDevName(devname);
	} else {
		data->setDevName(device);
	}

	// some assumptiond - should all be configurable
	data->setForceDownload(false);
	data->setSaveLog(false);
	data->setSaveDump(false);

	// before we start, remember where the dive_table ended
	diveImportedModel->startDownload();
}
