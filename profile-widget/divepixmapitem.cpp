// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/divepixmapitem.h"
#include "profile-widget/animationfunctions.h"
#include "qt-models/divepicturemodel.h"
#include "core/pref.h"
#ifndef SUBSURFACE_MOBILE
#include "desktop-widgets/preferences/preferencesdialog.h"
#endif

#include <QDesktopServices>
#include <QGraphicsView>
#include <QUrl>
#include <QGraphicsSceneMouseEvent>
#include <QMediaPlayer>

DivePixmapItem::DivePixmapItem(QGraphicsItem *parent) : QGraphicsPixmapItem(parent)
{
}

DiveButtonItem::DiveButtonItem(QGraphicsItem *parent): DivePixmapItem(parent)
{
}

void DiveButtonItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mousePressEvent(event);
	emit clicked();
}

CloseButtonItem::CloseButtonItem(QGraphicsItem *parent): DiveButtonItem(parent)
{
	static QPixmap p = QPixmap(":list-remove-icon");
	setPixmap(p);
	setFlag(ItemIgnoresTransformations);
}

void CloseButtonItem::hide()
{
	DiveButtonItem::hide();
}

void CloseButtonItem::show()
{
	DiveButtonItem::show();
}

DivePictureItem::DivePictureItem(QGraphicsItem *parent): DivePixmapItem(parent),
	canvas(new QGraphicsRectItem(this)),
	shadow(new QGraphicsRectItem(this)),
	button(new CloseButtonItem(this)),
	player(nullptr),
	video_projector(nullptr)
{
	setFlag(ItemIgnoresTransformations);
	setAcceptHoverEvents(true);
	setScale(0.2);
#ifndef SUBSURFACE_MOBILE
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
#endif
	setVisible(prefs.show_pictures_in_profile);

	canvas->setPen(Qt::NoPen);
	canvas->setBrush(QColor(Qt::white));
	canvas->setFlag(ItemStacksBehindParent);
	canvas->setZValue(-1);

	shadow->setPos(5,5);
	shadow->setPen(Qt::NoPen);
	shadow->setBrush(QColor(Qt::lightGray));
	shadow->setFlag(ItemStacksBehindParent);
	shadow->setZValue(-2);

	button->setScale(0.2);
	button->setZValue(7);
	button->hide();
}

void DivePictureItem::settingsChanged()
{
	setVisible(prefs.show_pictures_in_profile);
}

void DivePictureItem::setPixmap(const QPixmap &pix)
{
	DivePixmapItem::setPixmap(pix);
	updateSize(pix);
}

void DivePictureItem::updateSize(const QPixmap &pix)
{
	frameSize = pix.size();
	QRectF r = boundingRect();
	canvas->setRect(0 - 10, 0 -10, r.width() + 20, r.height() + 20);
	shadow->setRect(canvas->rect());
	button->setPos(r.width() - button->boundingRect().width() * 0.2,
			r.height() - button->boundingRect().height() * 0.2);
}

VideoProjector::VideoProjector(DivePictureItem *parent) : QAbstractVideoSurface(parent),
	divePictureItem(parent)
{
}

bool VideoProjector::present(const QVideoFrame &frame_in)
{
	QVideoFrame frame(frame_in);	// Copy so that we can map
	frame.map(QAbstractVideoBuffer::ReadOnly);
	QImage::Format format = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
	if (format == QImage::Format_Invalid)
		return false;
	QImage img(frame.bits(), frame.width(), frame.height(), format);
	divePictureItem->showVideoFrame(img);

	return true;
}

QList<QVideoFrame::PixelFormat> VideoProjector::supportedPixelFormats(QAbstractVideoBuffer::HandleType) const
{
	return {
		QVideoFrame::Format_ARGB32,
		QVideoFrame::Format_RGB32,
		QVideoFrame::Format_RGB24,
		QVideoFrame::Format_Jpeg
	};
}

void DivePictureItem::setVideo(const QString &filename, QGraphicsScene *scene)
{
	if (!video_projector) {
		video_projector = new VideoProjector(this);
		player = new QMediaPlayer(this);
		player->setVideoOutput(video_projector);
	}
	player->setMedia(QUrl::fromLocalFile(filename));
}

void DivePictureItem::showVideoFrame(const QImage &img)
{
	QPixmap pix = QPixmap::fromImage(img.scaled(frameSize, Qt::KeepAspectRatio));
	DivePixmapItem::setPixmap(pix);
	if (pix.size() != frameSize)
		// Ooops. Aspect ration changed. Move the close button, so that the
		// user can reach it without provoking the hoverLeaveEvent.
		updateSize(pix);
}

void DivePictureItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	Q_UNUSED(event);
	Animations::scaleTo(this, 1.0);
	setZValue(5);

	button->setOpacity(0);
	button->show();
	Animations::show(button);
	connect(button, SIGNAL(clicked()), this, SLOT(removePicture()));
	if (player)
		player->play();
}

void DivePictureItem::setFileUrl(const QString &s)
{
	fileUrl = s;
}

void DivePictureItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	Q_UNUSED(event);
	Animations::scaleTo(this, 0.2);
	setZValue(0);
	Animations::hide(button);
	if (player)
		player->stop();
}

void DivePictureItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		QDesktopServices::openUrl(QUrl::fromLocalFile(fileUrl));
	}
}

#ifndef SUBSURFACE_MOBILE
void DivePictureItem::removePicture()
{
	DivePictureModel::instance()->removePicture(fileUrl, true);
}
#endif
