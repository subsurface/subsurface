// SPDX-License-Identifier: GPL-2.0
#include "TabDivePhotos.h"
#include "ui_TabDivePhotos.h"
#include "core/imagedownloader.h"

#include <qt-models/divepicturemodel.h>

#include <QDesktopServices>
#include <QContextMenuEvent>
#include <QMenu>
#include <QUrl>
#include <QMessageBox>
#include <QFileInfo>
#include "core/save-profiledata.h"
#include "core/membuffer.h"

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
	ui->photosView->setResizeMode(QListView::Adjust);

	connect(ui->photosView, &DivePictureWidget::photoDoubleClicked,
		[](const QString& path) {
			QDesktopServices::openUrl(QUrl::fromLocalFile(path));
		}
	);
	connect(ui->photosView, &DivePictureWidget::zoomLevelChanged,
		this, &TabDivePhotos::changeZoomLevel);
	connect(ui->zoomSlider, &QAbstractSlider::valueChanged,
		DivePictureModel::instance(), &DivePictureModel::setZoomLevel);
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
	popup.addAction(tr("Load media from file(s)"), this, &TabDivePhotos::addPhotosFromFile);
	popup.addAction(tr("Load media file(s) from web"), this, &TabDivePhotos::addPhotosFromURL);
	popup.addSeparator();
	popup.addAction(tr("Delete selected media files"), this, &TabDivePhotos::removeSelectedPhotos);
	popup.addAction(tr("Delete all media files"), this, &TabDivePhotos::removeAllPhotos);
	popup.addAction(tr("Open folder of selected media files"), this, &TabDivePhotos::openFolderOfSelectedFiles);
	popup.addAction(tr("Recalculate selected thumbnails"), this, &TabDivePhotos::recalculateSelectedThumbnails);
	popup.addAction(tr("Save dive data as subtitles"), this, &TabDivePhotos::saveSubtitles);
	popup.exec(event->globalPos());
	event->accept();
}

QVector<QString> TabDivePhotos::getSelectedFilenames() const
{
	QVector<QString> selectedPhotos;
	if (!ui->photosView->selectionModel()->hasSelection())
		return selectedPhotos;
	QModelIndexList indexes = ui->photosView->selectionModel()->selectedRows();
	if (indexes.count() == 0)
		indexes = ui->photosView->selectionModel()->selectedIndexes();
	selectedPhotos.reserve(indexes.count());
	for (const auto &photo: indexes) {
		if (photo.isValid()) {
			QString fileUrl = photo.data(Qt::DisplayPropertyRole).toString();
			if (!fileUrl.isEmpty())
				selectedPhotos.push_back(fileUrl);
		}
	}
	return selectedPhotos;
}

void TabDivePhotos::removeSelectedPhotos()
{
	DivePictureModel::instance()->removePictures(getSelectedFilenames());
}

void TabDivePhotos::openFolderOfSelectedFiles()
{
	QVector<QString> directories;
	for (const QString &filename: getSelectedFilenames()) {
		QFileInfo info(filename);
		if (!info.exists())
			continue;
		QString path = info.absolutePath();
		if (path.isEmpty() || directories.contains(path))
			continue;
		directories.append(path);
	}
	for (const QString &dir: directories)
		QDesktopServices::openUrl(QUrl::fromLocalFile(dir));
}

void TabDivePhotos::recalculateSelectedThumbnails()
{
	Thumbnailer::instance()->calculateThumbnails(getSelectedFilenames());
}

void TabDivePhotos::saveSubtitles()
{
	QVector<QString> selectedPhotos;
	if (!ui->photosView->selectionModel()->hasSelection())
		return;
	QModelIndexList indexes = ui->photosView->selectionModel()->selectedRows();
	if (indexes.count() == 0)
		indexes = ui->photosView->selectionModel()->selectedIndexes();
	selectedPhotos.reserve(indexes.count());
	for (const auto &photo: indexes) {
		if (photo.isValid()) {
			QString fileUrl = photo.data(Qt::DisplayPropertyRole).toString();
			if (!fileUrl.isEmpty()) {
				QFileInfo fi = QFileInfo(fileUrl);
				QFile subtitlefile;
				subtitlefile.setFileName(QString(fi.path()) + "/" + fi.completeBaseName() + ".ass");
				int offset = photo.data(Qt::UserRole + 1).toInt();
				int duration = photo.data(Qt::UserRole + 2).toInt();
				// Only videos have non-zero duration
				if (!duration)
					continue;
				struct membuffer b = { 0 };
				save_subtitles_buffer(&b, &displayed_dive, offset, duration);
				char *data = detach_cstring(&b);
				subtitlefile.open(QIODevice::WriteOnly);
				subtitlefile.write(data, strlen(data));
				subtitlefile.close();
				free(data);
			}

		}
	}
}

//TODO: This looks overly wrong. We shouldn't call MainWindow to retrieve the DiveList to add Images.
void TabDivePhotos::addPhotosFromFile()
{
	MainWindow::instance()->diveList->loadImages();
}

void TabDivePhotos::addPhotosFromURL()
{
	MainWindow::instance()->diveList->loadWebImages();
}

void TabDivePhotos::removeAllPhotos()
{
	if (QMessageBox::warning(this, tr("Deleting media files"), tr("Are you sure you want to delete all media files?"), QMessageBox::Cancel | QMessageBox::Ok, QMessageBox::Cancel) != QMessageBox::Cancel ) {
		ui->photosView->selectAll();
		removeSelectedPhotos();
	}
}

void TabDivePhotos::updateData()
{
	divePictureModel->updateDivePictures();
}

void TabDivePhotos::changeZoomLevel(int delta)
{
	// We count on QSlider doing bound checks
	ui->zoomSlider->setValue(ui->zoomSlider->value() + delta);
}
