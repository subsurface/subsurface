#include "divepixmapitem.h"
#include "animationfunctions.h"
#include "divepicturemodel.h"
#include "pref.h"
#ifndef SUBSURFACE_MOBILE
#include "preferences/preferencesdialog.h"
#endif

#include <QDesktopServices>
#include <QGraphicsView>
#include <QUrl>

DivePixmapItem::DivePixmapItem(QObject *parent) : QObject(parent), QGraphicsPixmapItem()
{
}

DiveButtonItem::DiveButtonItem(QObject *parent): DivePixmapItem(parent)
{
}

void DiveButtonItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QGraphicsItem::mousePressEvent(event);
	emit clicked();
}

// If we have many many pictures on screen, maybe a shared-pixmap would be better to
// paint on screen, but for now, this.
CloseButtonItem::CloseButtonItem(QObject *parent): DiveButtonItem(parent)
{
	static QPixmap p = QPixmap(":trash");
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

DivePictureItem::DivePictureItem(QObject *parent): DivePixmapItem(parent),
	canvas(new QGraphicsRectItem(this)),
	shadow(new QGraphicsRectItem(this))
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
}

void DivePictureItem::settingsChanged()
{
	setVisible(prefs.show_pictures_in_profile);
}

void DivePictureItem::setPixmap(const QPixmap &pix)
{
	DivePixmapItem::setPixmap(pix);
	QRectF r = boundingRect();
	canvas->setRect(0 - 10, 0 -10, r.width() + 20, r.height() + 20);
	shadow->setRect(canvas->rect());
}

CloseButtonItem *button = NULL;
void DivePictureItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
	Animations::scaleTo(this, 1.0);
	setZValue(5);

	if(!button) {
		button = new CloseButtonItem();
		button->setScale(0.2);
		button->setZValue(7);
		scene()->addItem(button);
	}
	button->setParentItem(this);
	button->setPos(boundingRect().width() - button->boundingRect().width() * 0.2,
				   boundingRect().height() - button->boundingRect().height() * 0.2);
	button->setOpacity(0);
	button->show();
	Animations::show(button);
	button->disconnect();
	connect(button, SIGNAL(clicked()), this, SLOT(removePicture()));
}

void DivePictureItem::setFileUrl(const QString &s)
{
	fileUrl = s;
}

void DivePictureItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
	Animations::scaleTo(this, 0.2);
	setZValue(0);
	if(button){
		button->setParentItem(NULL);
		Animations::hide(button);
	}
}

DivePictureItem::~DivePictureItem(){
	if(button){
		button->setParentItem(NULL);
		Animations::hide(button);
	}
}

void DivePictureItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(fileUrl));
}

void DivePictureItem::removePicture()
{
	DivePictureModel::instance()->removePicture(fileUrl, true);
}
