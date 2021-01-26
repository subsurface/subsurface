// SPDX-License-Identifier: GPL-2.0
#include "qt-models/diveimportedmodel.h"

void cliDownloader(const char *vendor, const char *product, const char *device)
{
	DiveImportedModel diveImportedModel;
	DiveImportedModel::connect(&diveImportedModel, &DiveImportedModel::downloadFinished, [] {
		// do something useful at the end of the download
		printf("Finished\n");
	});

	auto data = diveImportedModel.thread.data();
	data->setVendor(vendor);
	data->setProduct(product);
	data->setBluetoothMode(false);
	if (data->vendor() == "Uemis") {
		QString devname(device);
		int colon = devname.indexOf(QStringLiteral(":\\ (UEMISSDA)"));
		if (colon >= 0) {
			devname.truncate(colon + 2);
			fprintf(stderr, "shortened devname to \"%s\"", qPrintable(devname));
		}
		data->setDevName(devname);
	} else {
		data->setDevName(device);
	}

	// some assumptions - should all be configurable
	data->setForceDownload(false);
	data->setSaveLog(true);
	data->setSaveDump(false);

	diveImportedModel.startDownload();
	diveImportedModel.waitForDownload();
	diveImportedModel.recordDives();
}
