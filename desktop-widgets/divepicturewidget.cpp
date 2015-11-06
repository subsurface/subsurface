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

DivePictureWidget::DivePictureWidget(QWidget *parent) : QListView(parent)
{
	connect(this, SIGNAL(doubleClicked(const QModelIndex &)), this, SLOT(doubleClicked(const QModelIndex &)));
}

void DivePictureWidget::doubleClicked(const QModelIndex &index)
{
	QString filePath = model()->data(index, Qt::DisplayPropertyRole).toString();
	emit photoDoubleClicked(localFilePath(filePath));
}
