#include "ruleritem.h"
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QGraphicsScene>

#include <stdint.h>

#include "profile.h"
#include "display.h"

RulerNodeItem2::RulerNodeItem2() : entry(NULL) , ruler(NULL)
{
	setRect(QRect(QPoint(-8,8),QPoint(8,-8)));
	setBrush(QColor(0xff, 0, 0, 127));
	setPen(QColor("#FF0000"));
	setFlag(QGraphicsItem::ItemIsMovable);
	setFlag(ItemSendsGeometryChanges);
	setFlag(ItemIgnoresTransformations);
}

void RulerNodeItem2::setRuler(RulerItem2 *r)
{
	ruler = r;
}

void RulerNodeItem2::recalculate()
{
	struct plot_data *data = pInfo->entry+(pInfo->nr-1);
	uint16_t count = 0;
	if (x() < 0) {
		setPos(0, y());
	} else if (x() > timeAxis->posAtValue(data->sec)) {
		setPos(timeAxis->posAtValue(data->sec), y());
	} else {
		data = pInfo->entry;
		count=0;
		while (timeAxis->posAtValue(data->sec) < x() && count < pInfo->nr) {
			data = pInfo->entry+count;
			count++;
		}
		setPos(timeAxis->posAtValue(data->sec), depthAxis->posAtValue(data->depth));
		entry=data;
	}
}

QVariant RulerNodeItem2::itemChange(GraphicsItemChange change, const QVariant &value)
{
	if (change == ItemPositionHasChanged) {
		recalculate();
		if (ruler != NULL)
			ruler->recalculate();
		if (scene()) {
			scene()->update();
		}
	}
	return QGraphicsEllipseItem::itemChange(change, value);
}

RulerItem2::RulerItem2():
	pInfo(NULL),
	timeAxis(NULL),
	depthAxis(NULL),
	source(new RulerNodeItem2()),
	dest(new RulerNodeItem2())
{

	source->setRuler(this);
	dest->setRuler(this);
}

void RulerItem2::recalculate()
{
	char buffer[500];
	QPointF tmp;
	QFont font;
	QFontMetrics fm(font);

	if (timeAxis == NULL || depthAxis == NULL || pInfo == NULL)
		return;

	prepareGeometryChange();
	startPoint = mapFromItem(source, 0, 0);
	endPoint = mapFromItem(dest, 0, 0);
	if (startPoint.x() > endPoint.x()) {
		tmp = endPoint;
		endPoint = startPoint;
		startPoint = tmp;
	}
	QLineF line(startPoint, endPoint);

	compare_samples(source->entry, dest->entry, buffer, 500, 1);
	text = QString(buffer);

	QRect r = fm.boundingRect(QRect(QPoint(10,-1*INT_MAX), QPoint(line.length()-10, 0)), Qt::TextWordWrap, text);
	if (r.height() < 10)
		height = 10;
	else
		height = r.height();

	QLineF line_n = line.normalVector();
	line_n.setLength(height);
	if (scene()) {
		/* Determine whether we draw down or upwards */
		if (scene()->sceneRect().contains(line_n.p2()) &&
		    scene()->sceneRect().contains(endPoint+QPointF(line_n.dx(),line_n.dy())))
			paint_direction = -1;
		else
			paint_direction = 1;
	}
}

RulerNodeItem2 *RulerItem2::sourceNode() const
{
	return source;
}

RulerNodeItem2 *RulerItem2::destNode() const
{
	return dest;
}

void RulerItem2::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	QLineF line(startPoint, endPoint);
	QLineF line_n = line.normalVector();
	painter->setPen(QColor(Qt::black));
	painter->setBrush(Qt::NoBrush);
	line_n.setLength(height);

	if (paint_direction == 1)
		line_n.setAngle(line_n.angle()+180);
	painter->drawLine(line);
	painter->drawLine(line_n);
	painter->drawLine(line_n.p1() + QPointF(line.dx(), line.dy()), line_n.p2() + QPointF(line.dx(), line.dy()));

	//Draw Text
	painter->save();
	painter->translate(startPoint.x(), startPoint.y());
	painter->rotate(line.angle()*-1);
	if (paint_direction == 1)
		painter->translate(0, height);
	painter->setPen(Qt::black);
	painter->drawText(QRectF(QPointF(10,-1*height), QPointF(line.length()-10, 0)), Qt::TextWordWrap, text);
	painter->restore();
}

QRectF RulerItem2::boundingRect() const
{
	return shape().controlPointRect();
}

QPainterPath RulerItem2::shape() const
{
	QPainterPath path;
	QLineF line(startPoint, endPoint);
	QLineF line_n = line.normalVector();
	line_n.setLength(height);
	if (paint_direction == 1)
		line_n.setAngle(line_n.angle()+180);
	path.moveTo(startPoint);
	path.lineTo(line_n.p2());
	path.lineTo(line_n.p2() + QPointF(line.dx(), line.dy()));
	path.lineTo(endPoint);
	path.lineTo(startPoint);
	return path;
}

void RulerItem2::setPlotInfo(plot_info* info)
{
	pInfo = info;
	dest->pInfo = info;
	source->pInfo = info;
	recalculate();
}
