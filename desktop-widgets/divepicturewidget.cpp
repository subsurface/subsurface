// SPDX-License-Identifier: GPL-2.0
#include "desktop-widgets/divepicturewidget.h"
#include "core/metrics.h"
#include "core/qthelper.h"
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QPixmap>

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
		QModelIndex index = indexAt(event->pos());
		QString filename = model()->data(index, Qt::DisplayPropertyRole).toString();

		if (!filename.isEmpty()) {
			int dim = lrint(defaultIconMetrics().sz_pic * 0.2);
			
			QPixmap pixmap = model()->data(indexAt(event->pos()), Qt::DecorationRole).value<QPixmap>();
			pixmap = pixmap.scaled(dim, dim, Qt::KeepAspectRatio);
			
			QByteArray itemData;
			QDataStream dataStream(&itemData, QIODevice::WriteOnly);
			dataStream << filename;

			QMimeData *mimeData = new QMimeData;
			mimeData->setData("application/x-subsurfaceimagedrop", itemData);

			QDrag *drag = new QDrag(this);
			drag->setMimeData(mimeData);
			drag->setPixmap(pixmap);

			drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
		}
	}
	QListView::mousePressEvent(event);
}

void DivePictureWidget::wheelEvent(QWheelEvent *event)
{
	if (event->modifiers() == Qt::ControlModifier) {
		// Angle delta is given in eighth parts of a degree. A classical mouse
		// wheel click is 15 degrees. Each click should correspond to one zoom step.
		// Therefore, divide by 15*8=120. To also support touch pads and finer-grained
		// mouse wheels, take care to always round away from zero.
		int delta = event->angleDelta().y();
		int carry = delta > 0 ? 119 : -119;
		emit zoomLevelChanged((delta + carry) / 120);
	} else
		QListView::wheelEvent(event);
}
