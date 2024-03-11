#include "informationbox.h"
#include "statscolors.h"
#include "zvalues.h"

#include <QFontMetrics>

static const int informationBorder = 2;
static const double informationBorderRadius = 4.0; // Radius of rounded corners
static const int distanceFromPointer = 10; // Distance to place box from mouse pointer or scatter item

InformationBox::InformationBox(ChartView &v, const StatsTheme &theme) :
	ChartRectItem(v, ChartZValue::InformationBox,
		      QPen(theme.informationBorderColor, informationBorder),
		      QBrush(theme.informationColor), informationBorderRadius),
	theme(theme),
	width(0.0),
	height(0.0)
{
}

void InformationBox::setText(const std::vector<QString> &text, QPointF pos)
{
	QFontMetrics fm(theme.informationBoxFont);
	double fontHeight = fm.height();

	std::vector<double> widths;
	widths.reserve(text.size());
	width = 0.0;
	for (const QString &s: text) {
		widths.push_back(static_cast<double>(fm.size(Qt::TextSingleLine, s).width()));
		width = std::max(width, widths.back());
	}

	width += 4.0 * informationBorder;
	height = widths.size() * fontHeight + 4.0 * informationBorder;

	ChartRectItem::resize(QSizeF(width, height));

	painter->setFont(theme.informationBoxFont);
	painter->setPen(QPen(theme.darkLabelColor)); // QPainter uses QPen to set text color!
	double y = 2.0 * informationBorder;
	for (size_t i = 0; i < widths.size(); ++i) {
		QRectF rect(2.0 * informationBorder, y, widths[i], fontHeight);
		painter->drawText(rect, text[i]);
		y += fontHeight;
	}
}

void InformationBox::setPos(QPointF pos)
{
	QSizeF size = sceneSize();

	double x = pos.x() + distanceFromPointer;
	if (x + width >= size.width()) {
		if (pos.x() - width >= 0.0)
			x = pos.x() - width;
		else
			x = pos.x() - width / 2.0;
	}
	double y = pos.y() + distanceFromPointer;
	if (y + height >= size.height()) {
		if (pos.y() - height >= 0.0)
			y = pos.y() - height;
		else
			y = pos.y() - height / 2.0;
	}

	ChartRectItem::setPos(QPointF(x, y));
}

// Try to stay within three-thirds of the chart height
int InformationBox::recommendedMaxLines() const
{
	QFontMetrics fm(theme.informationBoxFont);
	int maxHeight = static_cast<int>(sceneSize().height());
	return maxHeight * 2 / fm.height() / 3;
}
