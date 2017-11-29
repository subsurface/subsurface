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
	void mouseDoubleClickEvent(QMouseEvent *event) Q_DECL_OVERRIDE;
	void mousePressEvent(QMouseEvent *event) Q_DECL_OVERRIDE;

signals:
	void photoDoubleClicked(const QString filePath);
};

class DivePictureThumbnailThread : public QThread {
};

#endif
