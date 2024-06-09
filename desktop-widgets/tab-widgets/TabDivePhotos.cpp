// SPDX-License-Identifier: GPL-2.0
#include "TabDivePhotos.h"
#include "maintab.h"
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
#include "core/selection.h"

//TODO: Remove those in the future.
#include "../mainwindow.h"
#include "../divelistview.h"

TabDivePhotos::TabDivePhotos(MainTab *parent)
	: TabBase(parent),
	ui(new Ui::TabDivePhotos()),
	divePictureModel(DivePictureModel::instance())
{
	ui->setupUi(this);
	ui->photosView->setModel(divePictureModel);
	ui->photosView->setSelectionMode(QAbstractItemView::ExtendedSelection);
	ui->photosView->setSelectionBehavior(QAbstractItemView::SelectRows);
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
	// TODO: clear model
	divePictureModel->updateDivePictures();
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
	QModelIndexList indices = ui->photosView->selectionModel()->selectedRows();
	selectedPhotos.reserve(indices.count());
	for (const auto &photo: indices) {
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
	QModelIndexList indices = ui->photosView->selectionModel()->selectedRows();
	DivePictureModel::instance()->removePictures(indices);
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
	if (!parent.currentDive)
		return;
	if (!ui->photosView->selectionModel()->hasSelection())
		return;
	QModelIndexList indices = ui->photosView->selectionModel()->selectedRows();
	for (const auto &photo: indices) {
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
				std::string buffer = save_subtitles_buffer(parent.currentDive, offset, duration);
				subtitlefile.open(QIODevice::WriteOnly);
				subtitlefile.write(buffer.c_str(), buffer.size());
				subtitlefile.close();
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

void TabDivePhotos::updateData(const std::vector<dive *> &, dive *currentDive, int)
{
	// TODO: pass dive
	divePictureModel->updateDivePictures();
}

void TabDivePhotos::changeZoomLevel(int delta)
{
	// We count on QSlider doing bound checks
	ui->zoomSlider->setValue(ui->zoomSlider->value() + delta);
}
