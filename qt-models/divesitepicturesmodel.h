// SPDX-License-Identifier: GPL-2.0
#ifndef DIVESITEPICTURESMODEL_H
#define DIVESITEPICTURESMODEL_H

#include "divepicturemodel.h"

class DiveSitePicturesModel : public DivePictureModel {
	Q_OBJECT
public:
	static DiveSitePicturesModel *instance();
	void updateDivePictures();
private:
	DiveSitePicturesModel();
};

#endif
