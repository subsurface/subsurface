#include "informationbox.h"
#include "statscolors.h"
#include "zvalues.h"

#include <QFontMetrics>
#include <QGraphicsScene>

static const QColor informationBorderColor(Qt::black);
static const QColor informationColor(0xff, 0xff, 0x00, 192); // Note: fourth argument is opacity
static const int informationBorder = 2;
static const double informationBorderRadius = 4.0; // Radius of rounded corners
static const int distanceFromPointer = 10; // Distance to place box from mouse pointer or scatter item

InformationBox::InformationBox() : RoundRectItem(informationBorderRadius, nullptr)
{
	setPen(QPen(informationBorderColor, informationBorder));
	setBrush(informationColor);
	setZValue(ZValues::informationBox);
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
	QRectF plotArea = scene()->sceneRect();

	double x = pos.x() + distanceFromPointer;
	if (x + width >= plotArea.right()) {
		if (pos.x() - width >= plotArea.x())
			x = pos.x() - width;
		else
			x = pos.x() - width / 2.0;
	}
	double y = pos.y() + distanceFromPointer;
	if (y + height >= plotArea.bottom()) {
		if (pos.y() - height >= plotArea.y())
			y = pos.y() - height;
		else
			y = pos.y() - height / 2.0;
	}

	setRect(x, y, width, height);
	double actY = y + 2.0 * informationBorder;
	for (auto &item: textItems) {
		item->setPos(QPointF(x + 2.0 * informationBorder, actY));
		actY += item->boundingRect().height();
	}
}

void InformationBox::addLine(const QString &s)
{
	textItems.emplace_back(new QGraphicsSimpleTextItem(s, this));
	QGraphicsSimpleTextItem &item = *textItems.back();
	item.setBrush(QBrush(darkLabelColor));
	item.setPos(QPointF(0.0, height));
	item.setFont(font);
	item.setZValue(ZValues::informationBox);
	QRectF rect = item.boundingRect();
	width = std::max(width, rect.width());
	height += rect.height();
}

// Try to stay within three-thirds of the chart height
int InformationBox::recommendedMaxLines() const
{
	QFontMetrics fm(font);
	int maxHeight = static_cast<int>(scene()->sceneRect().height());
	return maxHeight * 2 / fm.height() / 3;
}
