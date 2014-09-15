#include "divetooltipitem.h"
#include "divecartesianaxis.h"
#include "profilewidget2.h"
#include "dive.h"
#include "profile.h"
#include "membuffer.h"
#include <QPropertyAnimation>
#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QBrush>
#include <QGraphicsScene>
#include <QSettings>
#include <QGraphicsView>
#include <QDebug>

#define PORT_IN_PROGRESS 1
#ifdef PORT_IN_PROGRESS
#include "display.h"
#endif

void ToolTipItem::addToolTip(const QString &toolTip, const QIcon &icon)
{
	QGraphicsPixmapItem *iconItem = 0;
	double yValue = title->boundingRect().height() + SPACING;
	Q_FOREACH (ToolTip t, toolTips) {
		yValue += t.second->boundingRect().height();
	}
	if (!icon.isNull()) {
		iconItem = new QGraphicsPixmapItem(icon.pixmap(ICON_SMALL, ICON_SMALL), this);
		iconItem->setPos(SPACING, yValue);
	}

	QGraphicsSimpleTextItem *textItem = new QGraphicsSimpleTextItem(toolTip, this);
	textItem->setPos(SPACING + ICON_SMALL + SPACING, yValue);
	textItem->setBrush(QBrush(Qt::white));
	textItem->setFlag(ItemIgnoresTransformations);
	toolTips.push_back(qMakePair(iconItem, textItem));
	expand();
}

void ToolTipItem::clear()
{
	Q_FOREACH (ToolTip t, toolTips) {
		delete t.first;
		delete t.second;
	}
	toolTips.clear();
}

void ToolTipItem::setRect(const QRectF &r)
{
	// qDeleteAll(childItems());
	delete background;

	rectangle = r;
	setBrush(QBrush(Qt::white));
	setPen(QPen(Qt::black, 0.5));

	// Creates a 2pixels border
	QPainterPath border;
	border.addRoundedRect(-4, -4, rectangle.width() + 8, rectangle.height() + 10, 3, 3);
	border.addRoundedRect(-1, -1, rectangle.width() + 3, rectangle.height() + 4, 3, 3);
	setPath(border);

	QPainterPath bg;
	bg.addRoundedRect(-1, -1, rectangle.width() + 3, rectangle.height() + 4, 3, 3);

	QColor c = QColor(Qt::black);
	c.setAlpha(155);

	QGraphicsPathItem *b = new QGraphicsPathItem(bg, this);
	b->setFlag(ItemStacksBehindParent);
	b->setFlag(ItemIgnoresTransformations);
	b->setBrush(c);
	b->setPen(QPen(QBrush(Qt::transparent), 0));
	b->setZValue(-10);
	background = b;

	updateTitlePosition();
}

void ToolTipItem::collapse()
{
	QPropertyAnimation *animation = new QPropertyAnimation(this, "rect");
	animation->setDuration(100);
	animation->setStartValue(nextRectangle);
	animation->setEndValue(QRect(0, 0, ICON_SMALL, ICON_SMALL));
	animation->start(QAbstractAnimation::DeleteWhenStopped);
	clear();

	status = COLLAPSED;
}

void ToolTipItem::expand()
{
	if (!title)
		return;

	double width = 0, height = title->boundingRect().height() + SPACING;
	Q_FOREACH (ToolTip t, toolTips) {
		if (t.second->boundingRect().width() > width)
			width = t.second->boundingRect().width();
		height += t.second->boundingRect().height();
	}
	/*       Left padding, Icon Size,   space, right padding */
	width += SPACING + ICON_SMALL + SPACING + SPACING;

	if (width < title->boundingRect().width() + SPACING * 2)
		width = title->boundingRect().width() + SPACING * 2;

	if (height < ICON_SMALL)
		height = ICON_SMALL;

	nextRectangle.setWidth(width);
	nextRectangle.setHeight(height);

	QPropertyAnimation *animation = new QPropertyAnimation(this, "rect");
	animation->setDuration(100);
	animation->setStartValue(rectangle);
	animation->setEndValue(nextRectangle);
	animation->start(QAbstractAnimation::DeleteWhenStopped);

	status = EXPANDED;
}

ToolTipItem::ToolTipItem(QGraphicsItem *parent) : QGraphicsPathItem(parent),
	background(0),
	separator(new QGraphicsLineItem(this)),
	title(new QGraphicsSimpleTextItem(tr("Information"), this)),
	status(COLLAPSED),
	timeAxis(0),
	lastTime(-1)
{
	memset(&pInfo, 0, sizeof(pInfo));

	setFlags(ItemIgnoresTransformations | ItemIsMovable | ItemClipsChildrenToShape);
	updateTitlePosition();
	setZValue(99);
}

ToolTipItem::~ToolTipItem()
{
	clear();
}

void ToolTipItem::updateTitlePosition()
{
	if (rectangle.width() < title->boundingRect().width() + SPACING * 4) {
		QRectF newRect = rectangle;
		newRect.setWidth(title->boundingRect().width() + SPACING * 4);
		newRect.setHeight((newRect.height() && isExpanded()) ? newRect.height() : ICON_SMALL);
		setRect(newRect);
	}

	title->setPos(boundingRect().width() / 2 - title->boundingRect().width() / 2 - 1, 0);
	title->setFlag(ItemIgnoresTransformations);
	title->setPen(QPen(Qt::white, 1));
	title->setBrush(Qt::white);

	if (toolTips.size() > 0) {
		double x1 = 3;
		double y1 = title->pos().y() + SPACING / 2 + title->boundingRect().height();
		double x2 = boundingRect().width() - 10;
		double y2 = y1;

		separator->setLine(x1, y1, x2, y2);
		separator->setFlag(ItemIgnoresTransformations);
		separator->setPen(QPen(Qt::white));
		separator->show();
	} else {
		separator->hide();
	}
}

bool ToolTipItem::isExpanded() const
{
	return status == EXPANDED;
}

void ToolTipItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
	persistPos();
	QGraphicsPathItem::mouseReleaseEvent(event);
	Q_FOREACH (QGraphicsItem *item, oldSelection) {
		item->setSelected(true);
	}
}

void ToolTipItem::persistPos()
{
	QSettings s;
	s.beginGroup("ProfileMap");
	s.setValue("tooltip_position", pos());
	s.endGroup();
}

void ToolTipItem::readPos()
{
	QSettings s;
	s.beginGroup("ProfileMap");
	QPointF value = s.value("tooltip_position").toPoint();
	if (!scene()->sceneRect().contains(value)) {
		value = QPointF(0, 0);
	}
	setPos(value);
}

void ToolTipItem::setPlotInfo(const plot_info &plot)
{
	pInfo = plot;
}

void ToolTipItem::setTimeAxis(DiveCartesianAxis *axis)
{
	timeAxis = axis;
}

void ToolTipItem::refresh(const QPointF &pos)
{
	int time = timeAxis->valueAt(pos);
	if (time == lastTime)
		return;

	lastTime = time;
	clear();
	struct membuffer mb = { 0 };

	get_plot_details_new(&pInfo, time, &mb);
	addToolTip(QString::fromUtf8(mb.buffer, mb.len));
	free_buffer(&mb);

	Q_FOREACH (QGraphicsItem *item, scene()->items(pos, Qt::IntersectsItemShape
		,Qt::DescendingOrder, scene()->views().first()->transform())) {
		if (!item->toolTip().isEmpty())
			addToolTip(item->toolTip());
	}
}

void ToolTipItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
	oldSelection = scene()->selectedItems();
	scene()->clearSelection();
	QGraphicsItem::mousePressEvent(event);
}
