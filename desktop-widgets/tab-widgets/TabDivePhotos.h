#ifndef TAB_DIVE_PHOTOS_H
#define TAB_DIVE_PHOTOS_H

#include "TabBase.h"

namespace Ui {
	class TabDivePhotos;
};

class DivePictureModel;

class TabDivePhotos : public TabBase {
	Q_OBJECT
public:
	TabDivePhotos(QWidget *parent = 0);
	~TabDivePhotos();
	void updateData() override;
	void clear() override;

protected:
	void contextMenuEvent(QContextMenuEvent *ev) override;

private:
	void addPhotosFromFile();
	void addPhotosFromURL();
	void removeAllPhotos();
	void removeSelectedPhotos();

	Ui::TabDivePhotos *ui;
	DivePictureModel *divePictureModel;
};

#endif
