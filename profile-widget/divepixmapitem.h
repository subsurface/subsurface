// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEPIXMAPITEM_H
#define DIVEPIXMAPITEM_H

#include <QObject>
#include <QGraphicsPixmapItem>
#include <QAbstractVideoSurface>
#include <QMediaPlayer>

class DivePixmapItem : public QObject, public QGraphicsPixmapItem {
	Q_OBJECT
	Q_PROPERTY(qreal opacity WRITE setOpacity READ opacity)
	Q_PROPERTY(QPointF pos WRITE setPos READ pos)
	Q_PROPERTY(qreal x WRITE setX READ x)
	Q_PROPERTY(qreal y WRITE setY READ y)
public:
	DivePixmapItem(QGraphicsItem *parent = 0);
};

class DiveButtonItem : public DivePixmapItem {
	Q_OBJECT
public:
	DiveButtonItem(QGraphicsItem *parent = 0);
protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
signals:
	void clicked();
};

class CloseButtonItem : public DiveButtonItem {
	Q_OBJECT
public:
	CloseButtonItem(QGraphicsItem *parent = 0);
public slots:
	void hide();
	void show();
};

class DivePictureItem;
class VideoProjector : public QAbstractVideoSurface {
	bool present(const QVideoFrame &frame);
	QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType) const;
	DivePictureItem *divePictureItem;
public:
	VideoProjector(DivePictureItem *);
};

class DivePictureItem : public DivePixmapItem {
	Q_OBJECT
	Q_PROPERTY(qreal scale WRITE setScale READ scale)
public:
	DivePictureItem(QGraphicsItem *parent = 0);
	void setPixmap(const QPixmap& pix);
	void setVideo(const QString &filename, QGraphicsScene *scene);
	void showVideoFrame(const QImage &img);
public slots:
	void settingsChanged();
#ifndef SUBSURFACE_MOBILE
	void removePicture();
#endif
	void setFileUrl(const QString& s);
protected:
	void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
	void mousePressEvent(QGraphicsSceneMouseEvent *event);
private:
	QString fileUrl;
	QGraphicsRectItem *canvas;
	QGraphicsRectItem *shadow;
	DiveButtonItem *button;
	QMediaPlayer *player;
	VideoProjector *video_projector;
	QSize frameSize;
	void updateSize(const QPixmap &pix);
};

#endif // DIVEPIXMAPITEM_H
