// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEPICTUREWIDGET_H
#define DIVEPICTUREWIDGET_H

#include <QAbstractTableModel>
#include <QListView>
#include <QThread>
#include <QFuture>

class DivePictureWidget : public QListView {
	Q_OBJECT
public:
	DivePictureWidget(QWidget *parent);
protected:
	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;

signals:
	void photoDoubleClicked(const QString filePath);
	void zoomLevelChanged(int delta);
};

class DivePictureThumbnailThread : public QThread {
};

#endif
