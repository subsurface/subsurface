// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/divepixmapitem.h"
#include "profile-widget/animationfunctions.h"
#include "qt-models/divepicturemodel.h"
#include "core/pref.h"
#include "core/qthelper.h"
#include "core/settings/qPrefDisplay.h"
#ifndef SUBSURFACE_MOBILE
#include "desktop-widgets/preferences/preferencesdialog.h"
#endif

#include <QDesktopServices>
#include <QGraphicsView>
#include <QUrl>
#include <QGraphicsSceneMouseEvent>

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
	baseZValue(0.0)
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

// The base z-value is used for correct paint-order of the thumbnails. On hoverEnter the z-value is raised
// so that the thumbnail is drawn on top of all other thumbnails and on hoverExit it is restored to the base value.
void DivePictureItem::setBaseZValue(double z)
{
	baseZValue = z;
	setZValue(z);
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
	button->setPos(boundingRect().width() - button->boundingRect().width() * 0.2,
				   boundingRect().height() - button->boundingRect().height() * 0.2);
}

void DivePictureItem::hoverEnterEvent(QGraphicsSceneHoverEvent*)
{
	Animations::scaleTo(this, qPrefDisplay::animation_speed(), 1.0);
	setZValue(baseZValue + 5.0);

	button->setOpacity(0);
	button->show();
	Animations::show(button, qPrefDisplay::animation_speed());
}

void DivePictureItem::setFileUrl(const QString &s)
{
	fileUrl = s;
}

void DivePictureItem::hoverLeaveEvent(QGraphicsSceneHoverEvent*)
{
	Animations::scaleTo(this, qPrefDisplay::animation_speed(), 0.2);
	setZValue(baseZValue);
	Animations::hide(button, qPrefDisplay::animation_speed());
}

void DivePictureItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		QDesktopServices::openUrl(QUrl::fromLocalFile(localFilePath(fileUrl)));
}

void DivePictureItem::removePicture()
{
#ifndef SUBSURFACE_MOBILE
	DivePictureModel::instance()->removePictures({ fileUrl });
#endif
}
