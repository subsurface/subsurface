// SPDX-License-Identifier: GPL-2.0
#include "TabDivePhotos.h"
#include "ui_TabDivePhotos.h"

#include <qt-models/divepicturemodel.h>

#include <QDesktopServices>
#include <QContextMenuEvent>
#include <QMenu>
#include <QUrl>
#include <QMessageBox>

//TODO: Remove those in the future.
#include "../mainwindow.h"
#include "../divelistview.h"

TabDivePhotos::TabDivePhotos(QWidget *parent)
	: TabBase(parent),
	ui(new Ui::TabDivePhotos()),
	divePictureModel(DivePictureModel::instance())
{
	ui->setupUi(this);
	ui->photosView->setModel(divePictureModel);
	ui->photosView->setSelectionMode(QAbstractItemView::ExtendedSelection);

	connect(ui->photosView, &DivePictureWidget::photoDoubleClicked,
		[](const QString& path) {
			QDesktopServices::openUrl(QUrl::fromLocalFile(path));
		}
	);
}

TabDivePhotos::~TabDivePhotos()
{
	delete ui;
}

void TabDivePhotos::clear()
{
	updateData();
}

void TabDivePhotos::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu popup(this);
	popup.addAction(tr("Load image(s) from file(s)"), this, SLOT(addPhotosFromFile()));
	popup.addAction(tr("Load image(s) from web"), this, SLOT(addPhotosFromURL()));
	popup.addSeparator();
	popup.addAction(tr("Delete selected images"), this, SLOT(removeSelectedPhotos()));
	popup.addAction(tr("Delete all images"), this, SLOT(removeAllPhotos()));
	popup.exec(event->globalPos());
	event->accept();
}

void TabDivePhotos::removeSelectedPhotos()
{
	bool last = false;
	if (!ui->photosView->selectionModel()->hasSelection())
		return;
	QModelIndexList indexes =  ui->photosView->selectionModel()->selectedRows();
	if (indexes.count() == 0)
		indexes = ui->photosView->selectionModel()->selectedIndexes();
	QModelIndex photo = indexes.first();
	do {
		photo = indexes.first();
		last = indexes.count() == 1;
		if (photo.isValid()) {
			QString fileUrl = photo.data(Qt::DisplayPropertyRole).toString();
			if (fileUrl.length() > 0)
				DivePictureModel::instance()->removePicture(fileUrl, last);
		}
		indexes.removeFirst();
	} while(!indexes.isEmpty());
}

//TODO: This looks overly wrong. We shouldn't call MainWindow to retrieve the DiveList to add Images.
void TabDivePhotos::addPhotosFromFile()
{
	MainWindow::instance()->dive_list()->loadImages();
}

void TabDivePhotos::addPhotosFromURL()
{
	MainWindow::instance()->dive_list()->loadWebImages();
}

void TabDivePhotos::removeAllPhotos()
{
	if (QMessageBox::warning(this, tr("Deleting Images"), tr("Are you sure you want to delete all images?"), QMessageBox::Cancel | QMessageBox::Ok, QMessageBox::Cancel) != QMessageBox::Cancel ) {
		ui->photosView->selectAll();
		removeSelectedPhotos();
	}
}

void TabDivePhotos::updateData()
{
	divePictureModel->updateDivePictures();
}

