// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/divepixmapitem.h"
#include "profile-widget/animationfunctions.h"
#include "qt-models/divepicturemodel.h"
#include "core/pref.h"
#include "core/qthelper.h"
#ifndef SUBSURFACE_MOBILE
#include "desktop-widgets/preferences/preferencesdialog.h"
#endif

#include <QDesktopServices>
#include <QGraphicsView>
#include <QUrl>
#include <QGraphicsSceneMouseEvent>
#ifndef SUBSURFACE_MOBILE
#include <QMediaPlayer>
#endif

DivePixmapItem::DivePixmapItem(QGraphicsItem *parent) : QGraphicsPixmapItem(parent)
{
}

CloseButtonItem::CloseButtonItem(QGraphicsItem *parent): DivePixmapItem(parent)
{
	static QPixmap p = QPixmap(":list-remove-icon");
	setPixmap(p);
	setFlag(ItemIgnoresTransformations);
}

void CloseButtonItem::mousePressEvent(QGraphicsSceneMouseEvent *)
{
	qgraphicsitem_cast<DivePictureItem*>(parentItem())->removePicture();
}

void CloseButtonItem::hide()
{
	DivePixmapItem::hide();
}

void CloseButtonItem::show()
{
	DivePixmapItem::show();
}

DivePictureItem::DivePictureItem(QGraphicsItem *parent): DivePixmapItem(parent),
	canvas(new QGraphicsRectItem(this)),
	shadow(new QGraphicsRectItem(this)),
	button(new CloseButtonItem(this)),
	isVideo(false)
{
	setFlag(ItemIgnoresTransformations);
	setAcceptHoverEvents(true);
	setScale(0.2);
#ifndef SUBSURFACE_MOBILE
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));
#endif

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

#ifndef SUBSURFACE_MOBILE
VideoProjector::VideoProjector(DivePictureItem *parent) : QAbstractVideoSurface(parent),
	divePictureItem(parent)
{
}

bool VideoProjector::present(const QVideoFrame &frame_in)
{
	QVideoFrame frame(frame_in);	// Copy so that we can map
	if (!frame.map(QAbstractVideoBuffer::ReadOnly))
		return false;
	QImage::Format format = QVideoFrame::imageFormatFromPixelFormat(frame.pixelFormat());
	if (format == QImage::Format_Invalid)
		return false;
	QImage img(frame.bits(), frame.width(), frame.height(), format);
	divePictureItem->showVideoFrame(img);
	frame.unmap();			// TODO: Necessary? Should be done by destructor?

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
#endif

void DivePictureItem::setVideo()
{
	isVideo = true;
}

void DivePictureItem::showVideoFrame(const QImage &img)
{
	QPixmap pix = QPixmap::fromImage(img.scaled(frameSize, Qt::KeepAspectRatio));
	DivePixmapItem::setPixmap(pix);
	if (pix.size() != frameSize)
		// Ooops. Aspect ratio changed. Move the close button, so that the
		// user can reach it without provoking the hoverLeaveEvent.
		updateSize(pix);
}

void DivePictureItem::stopVideo()
{
#ifndef SUBSURFACE_MOBILE
	if (video_projector) {
		player->stop();
		player.reset();
		video_projector.reset();
	}
#endif
}

void DivePictureItem::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
	Animations::scaleTo(this, 1.0);
	setZValue(5);

	button->setOpacity(0);
	button->show();
	Animations::show(button);
#ifndef SUBSURFACE_MOBILE
	if (isVideo) {
		if (!video_projector) {
			video_projector.reset(new VideoProjector(this));
			player.reset(new QMediaPlayer);
			player->setVideoOutput(&*video_projector);
		}
		player->setMedia(QUrl::fromLocalFile(fileUrl));
		player->play();
	}
#endif
}

void DivePictureItem::setFileUrl(const QString &s)
{
	fileUrl = s;
}

void DivePictureItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
	Animations::scaleTo(this, 0.2);
	setZValue(0);
	Animations::hide(button);
	stopVideo();
}

void DivePictureItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		// On click, open file in the system viewer.
		// If this is a video, stop playing so that the user doesn't have two videos
		// playing at the same time.
		stopVideo();
		QDesktopServices::openUrl(QUrl::fromLocalFile(localFilePath(fileUrl)));
	}
}

void DivePictureItem::removePicture()
{
#ifndef SUBSURFACE_MOBILE
	DivePictureModel::instance()->removePictures({ fileUrl });
#endif
}
