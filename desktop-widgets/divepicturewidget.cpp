// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/divepicturewidget.h"
#include "qt-models/divepicturemodel.h"
#include "core/metrics.h"
#include "core/dive.h"
#include "core/divelist.h"
#include <unistd.h>
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <QFuture>
#include <QDir>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "desktop-widgets/mainwindow.h"
#include "core/qthelper.h"
#include <QStandardPaths>
#include <QtWidgets>

DivePictureWidget::DivePictureWidget(QWidget *parent) : QListView(parent)
{
}

void DivePictureWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		QString filePath = model()->data(indexAt(event->pos()), Qt::DisplayPropertyRole).toString();
		emit photoDoubleClicked(localFilePath(filePath));
	}
}

void DivePictureWidget::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton && event->modifiers() == Qt::NoModifier) {
		QString filename = model()->data(indexAt(event->pos()), Qt::DisplayPropertyRole).toString();

		if (!filename.isEmpty()) {
			int dim = lrint(defaultIconMetrics().sz_pic * 0.2);
			
			QPixmap pixmap = model()->data(indexAt(event->pos()), Qt::DecorationRole).value<QPixmap>();
			pixmap = pixmap.scaled(dim, dim, Qt::KeepAspectRatio);
			
			QByteArray itemData;
			QDataStream dataStream(&itemData, QIODevice::WriteOnly);
			dataStream << filename << event->pos();

			QMimeData *mimeData = new QMimeData;
			mimeData->setData("application/x-subsurfaceimagedrop", itemData);

			QDrag *drag = new QDrag(this);
			drag->setMimeData(mimeData);
			drag->setPixmap(pixmap);

			drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
		}

		QListView::mousePressEvent(event);
	} else {
		QListView::mousePressEvent(event);
	}
}
