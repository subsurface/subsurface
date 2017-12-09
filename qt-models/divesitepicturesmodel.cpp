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
	pictures.clear();
	endResetModel();

	const uint32_t ds_uuid = displayed_dive_site.uuid;
	struct dive *d;
	int i;

	for_each_dive (i, d)
		if (d->dive_site_uuid == ds_uuid && dive_get_picture_count(d))
			FOR_EACH_PICTURE(d)
				pictures.push_back({picture, picture->filename, QImage(), picture->offset.seconds});

	QtConcurrent::blockingMap(pictures, scaleImages);

	beginInsertRows(QModelIndex(), 0, pictures.count() - 1);
	endInsertRows();
}
