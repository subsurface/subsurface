// SPDX-License-Identifier: GPL-2.0
#include "qt-models/divesitepicturesmodel.h"
#include "core/dive.h"
#include <stdint.h>

#include <QtConcurrent>
#include <QPixmap>

DiveSitePicturesModel* DiveSitePicturesModel::instance() {
	static DiveSitePicturesModel *self = new DiveSitePicturesModel();
	return self;
}

DiveSitePicturesModel::DiveSitePicturesModel() {

}

void DiveSitePicturesModel::updateDivePictures() {
	beginResetModel();
	numberOfPictures = 0;
	endResetModel();

	const uint32_t ds_uuid = displayed_dive_site.uuid;
	struct dive *d;
	int i;

	stringPixmapCache.clear();
	SPictureList pictures;

	for_each_dive (i, d) {
		if (d->dive_site_uuid == ds_uuid && dive_get_picture_count(d)) {
			FOR_EACH_PICTURE(d) {
				stringPixmapCache[QString(picture->filename)].offsetSeconds = picture->offset.seconds;
				pictures.push_back(picture);
			}
		}
	}

	QList<SPixmap> list = QtConcurrent::blockingMapped(pictures, scaleImages);
	Q_FOREACH (const SPixmap &pixmap, list)
		stringPixmapCache[pixmap.first->filename].image = pixmap.second;

	numberOfPictures = list.count();
	beginInsertRows(QModelIndex(), 0, numberOfPictures - 1);
	endInsertRows();
}
