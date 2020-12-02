#include "informationbox.h"
#include <QChart>

static const QColor informationBorderColor(Qt::black);
static const QColor informationColor(0xff, 0xff, 0x00, 192); // Note: fourth argument is opacity
static const int informationBorder = 2;
static int distanceFromPointer = 10; // Distance to place box from mouse pointer or scatter item

InformationBox::InformationBox(QtCharts::QChart *chart) : QGraphicsRectItem(chart), chart(chart)
{
	setPen(QPen(informationBorderColor, informationBorder));
	setBrush(informationColor);
}

void InformationBox::setText(const std::vector<QString> &text, QPointF pos)
{
	width = height = 0.0;
	textItems.clear();

	for (const QString &s: text) {
		if (!s.isEmpty())
			addLine(s);
	}

	width += 4.0 * informationBorder;
	height += 4.0 * informationBorder;

	// Setting the position will also set the proper size
	setPos(pos);
}

void InformationBox::setPos(QPointF pos)
{
	QRectF plotArea = chart->plotArea();

	double x = pos.x() + distanceFromPointer;
	if (x + width >= plotArea.right() && pos.x() - width >= plotArea.x())
		x = pos.x() - width;
	double y = pos.y() + distanceFromPointer;
	if (y + height >= plotArea.bottom() && pos.y() - height >= plotArea.y())
		y = pos.y() - height;

	setRect(x, y, width, height);
	double actY = y + 2.0 * informationBorder;
	for (auto &item: textItems) {
		item->setPos(QPointF(x + 2.0 * informationBorder, actY));
		actY += item->boundingRect().height();
	}
	setZValue(20.0); // What would a proper value be here?
}

void InformationBox::addLine(const QString &s)
{
	textItems.emplace_back(new QGraphicsSimpleTextItem(s, this));
	QGraphicsSimpleTextItem &item = *textItems.back();
	item.setPos(QPointF(0.0, height));
	QRectF rect = item.boundingRect();
	width = std::max(width, rect.width());
	height += rect.height();
}
