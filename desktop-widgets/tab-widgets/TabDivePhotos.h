// SPDX-License-Identifier: GPL-2.0
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
	TabDivePhotos(MainTab *parent);
	~TabDivePhotos();
	void updateData(const std::vector<dive *> &selection, dive *currentDive, int currentDC) override;
	void clear() override;

protected:
	void contextMenuEvent(QContextMenuEvent *ev) override;

private slots:
	void addPhotosFromFile();
	void addPhotosFromURL();
	void removeAllPhotos();
	void removeSelectedPhotos();
	void recalculateSelectedThumbnails();
	void openFolderOfSelectedFiles();
	void changeZoomLevel(int delta);
	void saveSubtitles();

private:
	Ui::TabDivePhotos *ui;
	DivePictureModel *divePictureModel;
	QVector<QString> getSelectedFilenames() const;
};

#endif
