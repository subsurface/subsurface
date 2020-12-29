// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/divecartesianaxis.h"
#include "profile-widget/divetextitem.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include "core/subsurface-qt/divelistnotifier.h"
#include "qt-models/diveplotdatamodel.h"
#include "profile-widget/animationfunctions.h"
#include "profile-widget/divelineitem.h"
#include "profile-widget/profilewidget2.h"

QPen DiveCartesianAxis::gridPen() const
{
	QPen pen;
	pen.setColor(getColor(TIME_GRID));
	/* cosmetic width() == 0 for lines in printMode
	 * having setCosmetic(true) and width() > 0 does not work when
	 * printing on OSX and Linux */
	pen.setWidth(DiveCartesianAxis::printMode ? 0 : 2);
	pen.setCosmetic(true);
	return pen;
}

void DiveCartesianAxis::setFontLabelScale(qreal scale)
{
	labelScale = scale;
	changed = true;
}

void DiveCartesianAxis::setPrintMode(bool mode)
{
	printMode = mode;
	// update the QPen of all lines depending on printMode
	QPen newPen = gridPen();
	QColor oldColor = pen().brush().color();
	newPen.setBrush(oldColor);
	setPen(newPen);
	Q_FOREACH (DiveLineItem *item, lines)
		item->setPen(pen());
}

void DiveCartesianAxis::setMaximum(double maximum)
{
	if (IS_FP_SAME(max, maximum))
		return;
	max = maximum;
	changed = true;
	emit maxChanged();
}

void DiveCartesianAxis::setMinimum(double minimum)
{
	if (IS_FP_SAME(min, minimum))
		return;
	min = minimum;
	changed = true;
}

void DiveCartesianAxis::setTextColor(const QColor &color)
{
	textColor = color;
}

DiveCartesianAxis::DiveCartesianAxis(ProfileWidget2 *widget) : QObject(),
	QGraphicsLineItem(),
	printMode(false),
	unitSystem(0),
	profileWidget(widget),
	orientation(LeftToRight),
	min(0),
	max(0),
	interval(1),
	tick_size(0),
	textVisibility(true),
	lineVisibility(true),
	labelScale(1.0),
	line_size(1),
	changed(true)
{
	setPen(gridPen());
}

DiveCartesianAxis::~DiveCartesianAxis()
{
}

void DiveCartesianAxis::setLineSize(qreal lineSize)
{
	line_size = lineSize;
	changed = true;
}

void DiveCartesianAxis::setOrientation(Orientation o)
{
	orientation = o;
	changed = true;
}

QColor DiveCartesianAxis::colorForValue(double) const
{
	return QColor(Qt::black);
}

void DiveCartesianAxis::setTextVisible(bool arg1)
{
	if (textVisibility == arg1) {
		return;
	}
	textVisibility = arg1;
	Q_FOREACH (DiveTextItem *item, labels) {
		item->setVisible(textVisibility);
	}
}

void DiveCartesianAxis::setLinesVisible(bool arg1)
{
	if (lineVisibility == arg1) {
		return;
	}
	lineVisibility = arg1;
	Q_FOREACH (DiveLineItem *item, lines) {
		item->setVisible(lineVisibility);
	}
}

template <typename T>
void emptyList(QList<T *> &list, int steps, int speed)
{
	while (list.size() > steps) {
		T *removedItem = list.takeLast();
		Animations::animDelete(removedItem, speed);
	}
}

void DiveCartesianAxis::updateTicks(color_index_t color)
{
	if (!scene() || (!changed && !profileWidget->getPrintMode()))
		return;
	QLineF m = line();
	// unused so far:
	// QGraphicsView *view = scene()->views().first();
	double stepsInRange = (max - min) / interval;
	int steps = (int)stepsInRange;
	double currValueText = min;
	double currValueLine = min;

	if (steps < 1)
		return;

	emptyList(labels, steps, profileWidget->animSpeed);
	emptyList(lines, steps, profileWidget->animSpeed);

	// Move the remaining ticks / text to their correct positions
	// regarding the possible new values for the axis
	qreal begin, stepSize;
	if (orientation == TopToBottom) {
		begin = m.y1();
		stepSize = (m.y2() - m.y1());
	} else if (orientation == BottomToTop) {
		begin = m.y2();
		stepSize = (m.y2() - m.y1());
	} else if (orientation == LeftToRight) {
		begin = m.x1();
		stepSize = (m.x2() - m.x1());
	} else /* if (orientation == RightToLeft) */ {
		begin = m.x2();
		stepSize = (m.x2() - m.x1());
	}
	stepSize /= stepsInRange;

	for (int i = 0, count = labels.size(); i < count; i++, currValueText += interval) {
		qreal childPos = (orientation == TopToBottom || orientation == LeftToRight) ?
					 begin + i * stepSize :
					 begin - i * stepSize;

		labels[i]->setText(textForValue(currValueText));
		if (orientation == LeftToRight || orientation == RightToLeft) {
			Animations::moveTo(labels[i], profileWidget->animSpeed, childPos, m.y1() + tick_size);
		} else {
			Animations::moveTo(labels[i], profileWidget->animSpeed ,m.x1() - tick_size, childPos);
		}
	}

	for (int i = 0, count = lines.size(); i < count; i++, currValueLine += interval) {
		qreal childPos = (orientation == TopToBottom || orientation == LeftToRight) ?
					 begin + i * stepSize :
					 begin - i * stepSize;

		if (orientation == LeftToRight || orientation == RightToLeft) {
			Animations::moveTo(lines[i], profileWidget->animSpeed, childPos, m.y1());
		} else {
			Animations::moveTo(lines[i], profileWidget->animSpeed, m.x1(), childPos);
		}
	}

	// Add's the rest of the needed Ticks / Text.
	for (int i = labels.size(); i < steps; i++, currValueText += interval) {
		qreal childPos;
		if (orientation == TopToBottom || orientation == LeftToRight) {
			childPos = begin + i * stepSize;
		} else {
			childPos = begin - i * stepSize;
		}
		DiveTextItem *label = new DiveTextItem(this);
		label->setText(textForValue(currValueText));
		label->setBrush(colorForValue(currValueText));
		label->setScale(fontLabelScale());
		label->setZValue(1);
		labels.push_back(label);
		if (orientation == RightToLeft || orientation == LeftToRight) {
			label->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
			label->setPos(scene()->sceneRect().width() + 10, m.y1() + tick_size); // position it outside of the scene);
			Animations::moveTo(label, profileWidget->animSpeed,childPos , m.y1() + tick_size);
		} else {
			label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
			label->setPos(m.x1() - tick_size, scene()->sceneRect().height() + 10);
			Animations::moveTo(label, profileWidget->animSpeed, m.x1() - tick_size, childPos);
		}
	}

	// Add's the rest of the needed Ticks / Text.
	for (int i = lines.size(); i < steps; i++, currValueText += interval) {
		qreal childPos;
		if (orientation == TopToBottom || orientation == LeftToRight) {
			childPos = begin + i * stepSize;
		} else {
			childPos = begin - i * stepSize;
		}
		DiveLineItem *line = new DiveLineItem(this);
		QPen pen = gridPen();
		pen.setBrush(getColor(color));
		line->setPen(pen);
		line->setZValue(0);
		lines.push_back(line);
		if (orientation == RightToLeft || orientation == LeftToRight) {
			line->setLine(0, -line_size, 0, 0);
			line->setPos(scene()->sceneRect().width() + 10, m.y1()); // position it outside of the scene);
			Animations::moveTo(line, profileWidget->animSpeed, childPos, m.y1());
		} else {
			QPointF p1 = mapFromScene(3, 0);
			QPointF p2 = mapFromScene(line_size, 0);
			line->setLine(p1.x(), 0, p2.x(), 0);
			line->setPos(m.x1(), scene()->sceneRect().height() + 10);
			Animations::moveTo(line, profileWidget->animSpeed, m.x1(), childPos);
		}
	}

	Q_FOREACH (DiveTextItem *item, labels)
		item->setVisible(textVisibility);
	Q_FOREACH (DiveLineItem *item, lines)
		item->setVisible(lineVisibility);
	changed = false;
}

void DiveCartesianAxis::setLine(const QLineF &line)
{
	QGraphicsLineItem::setLine(line);
	changed = true;
}

void DiveCartesianAxis::animateChangeLine(const QLineF &newLine)
{
	setLine(newLine);
	updateTicks();
	sizeChanged();
}

QString DiveCartesianAxis::textForValue(double value) const
{
	return QString("%L1").arg(value, 0, 'g', 4);
}

void DiveCartesianAxis::setTickSize(qreal size)
{
	tick_size = size;
}

void DiveCartesianAxis::setTickInterval(double i)
{
	interval = i;
}

qreal DiveCartesianAxis::valueAt(const QPointF &p) const
{
	double fraction;
	QLineF m = line();
	QPointF relativePosition = p;
	relativePosition -= pos(); // normalize p based on the axis' offset on screen

	if (orientation == LeftToRight || orientation == RightToLeft)
		fraction = (relativePosition.x() - m.x1()) / (m.x2() - m.x1());
	else
		fraction = (relativePosition.y() - m.y1()) / (m.y2() - m.y1());

	if (orientation == RightToLeft || orientation == BottomToTop)
			fraction = 1 - fraction;
	return fraction * (max - min) + min;
}

qreal DiveCartesianAxis::posAtValue(qreal value) const
{
	QLineF m = line();
	QPointF p = pos();

	double size = max - min;
	// unused for now:
	// double distanceFromOrigin = value - min;
	double percent = IS_FP_SAME(min, max) ? 0.0 : (value - min) / size;


	double realSize = orientation == LeftToRight || orientation == RightToLeft ?
				  m.x2() - m.x1() :
				  m.y2() - m.y1();

	// Inverted axis, just invert the percentage.
	if (orientation == RightToLeft || orientation == BottomToTop)
		percent = 1 - percent;

	double retValue = realSize * percent;
	double adjusted =
		orientation == LeftToRight ? retValue + m.x1() + p.x() :
		orientation == RightToLeft ? retValue + m.x1() + p.x() :
		orientation == TopToBottom ? retValue + m.y1() + p.y() :
		/* entation == BottomToTop */ retValue + m.y1() + p.y();
	return adjusted;
}

double DiveCartesianAxis::maximum() const
{
	return max;
}

double DiveCartesianAxis::minimum() const
{
	return min;
}

double DiveCartesianAxis::fontLabelScale() const
{
	return labelScale;
}

void DiveCartesianAxis::setColor(const QColor &color)
{
	QPen defaultPen = gridPen();
	defaultPen.setColor(color);
	defaultPen.setJoinStyle(Qt::RoundJoin);
	defaultPen.setCapStyle(Qt::RoundCap);
	setPen(defaultPen);
}

QString DepthAxis::textForValue(double value) const
{
	if (value == 0)
		return QString();
	return get_depth_string(lrint(value), false, false);
}

QColor DepthAxis::colorForValue(double) const
{
	return QColor(Qt::red);
}

DepthAxis::DepthAxis(ProfileWidget2 *widget) : DiveCartesianAxis(widget),
	unitSystem(prefs.units.length)
{
	connect(&diveListNotifier, &DiveListNotifier::settingsChanged, this, &DepthAxis::settingsChanged);
	changed = true;
}

void DepthAxis::settingsChanged()
{
	if (unitSystem == prefs.units.length)
		return;
	changed = true;
	updateTicks();
	unitSystem = prefs.units.length;
}

TimeAxis::TimeAxis(ProfileWidget2 *widget) : DiveCartesianAxis(widget)
{
}

QColor TimeAxis::colorForValue(double) const
{
	return QColor(Qt::blue);
}

QString TimeAxis::textForValue(double value) const
{
	int nr = lrint(value) / 60;
	if (maximum() < 600)
		return QString("%1:%2").arg(nr).arg((int)value % 60, 2, 10, QChar('0'));
	return QString::number(nr);
}

void TimeAxis::updateTicks(color_index_t color)
{
	DiveCartesianAxis::updateTicks(color);
	if (maximum() > 600) {
		for (int i = 0; i < labels.count(); i++) {
			labels[i]->setVisible(i % 2);
		}
	}
}

TemperatureAxis::TemperatureAxis(ProfileWidget2 *widget) : DiveCartesianAxis(widget)
{
}

QString TemperatureAxis::textForValue(double value) const
{
	return QString::number(mkelvin_to_C((int)value));
}

PartialGasPressureAxis::PartialGasPressureAxis(const DivePlotDataModel &model, ProfileWidget2 *widget) :
	DiveCartesianAxis(widget),
	model(model)
{
	connect(&diveListNotifier, &DiveListNotifier::settingsChanged, this, &PartialGasPressureAxis::update);
}

void PartialGasPressureAxis::update()
{
	bool showPhe = prefs.pp_graphs.phe;
	bool showPn2 = prefs.pp_graphs.pn2;
	bool showPo2 = prefs.pp_graphs.po2;
	setVisible(showPhe || showPn2 || showPo2);
	if (!model.rowCount())
		return;

	double max = showPhe ? model.pheMax() : -1;
	if (showPn2 && model.pn2Max() > max)
		max = model.pn2Max();
	if (showPo2 && model.po2Max() > max)
		max = model.po2Max();

	qreal pp = floor(max * 10.0) / 10.0 + 0.2;
	if (IS_FP_SAME(maximum(), pp))
		return;

	setMaximum(pp);
	setTickInterval(pp > 4 ? 0.5 : 0.25);
	updateTicks();
}
