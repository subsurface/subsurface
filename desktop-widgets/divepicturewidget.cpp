#include "divepicturewidget.h"
#include "divepicturemodel.h"
#include "metrics.h"
#include "dive.h"
#include "divelist.h"
#include <unistd.h>
#include <QtConcurrentMap>
#include <QtConcurrentRun>
#include <QFuture>
#include <QDir>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <mainwindow.h>
#include <qthelper.h>
#include <QStandardPaths>
#include <QtWidgets>

DivePictureWidget::DivePictureWidget(QWidget *parent) : QListView(parent)
{
	connect(this, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(doubleClicked(const QModelIndex &)));
}

void DivePictureWidget::doubleClicked(const QModelIndex &index)
{
	QString filePath = model()->data(index, Qt::DisplayPropertyRole).toString();
	emit photoDoubleClicked(localFilePath(filePath));
}


void DivePictureWidget::mousePressEvent(QMouseEvent *event)
{

	QPixmap pixmap = model()->data(indexAt(event->pos()), Qt::DecorationRole).value<QPixmap>();

	QString filename = model()->data(indexAt(event->pos()), Qt::DisplayPropertyRole).toString();

	QByteArray itemData;
	QDataStream dataStream(&itemData, QIODevice::WriteOnly);
	dataStream << filename << event->pos();

	QMimeData *mimeData = new QMimeData;
	mimeData->setData("application/x-subsurfaceimagedrop", itemData);

	QDrag *drag = new QDrag(this);
	drag->setMimeData(mimeData);
	drag->setPixmap(pixmap);
	drag->setHotSpot(event->pos() - rectForIndex(indexAt(event->pos())).topLeft());

	QPixmap tempPixmap = pixmap;
	QPainter painter;
	painter.begin(&tempPixmap);
	painter.fillRect(pixmap.rect(), QColor(127, 127, 127, 127));
	painter.end();

	drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
}
