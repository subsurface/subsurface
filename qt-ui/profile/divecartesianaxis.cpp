#include "divecartesianaxis.h"
#include "divelineitem.h"
#include "divetextitem.h"
#include "helpers.h"
#include "preferences.h"
#include "diveplotdatamodel.h"
#include "animationfunctions.h"
#include <QPen>
#include <QGraphicsScene>
#include <QDebug>
#include <QGraphicsView>
#include <QStyleOption>
#include <QSettings>

static QPen gridPen()
{
	QPen pen;
	pen.setColor(getColor(TIME_GRID));
	pen.setWidth(2);
	pen.setCosmetic(true);
	return pen;
}

double DiveCartesianAxis::tickInterval() const
{
	return interval;
}

double DiveCartesianAxis::tickSize() const
{
	return tick_size;
}

void DiveCartesianAxis::setFontLabelScale(qreal scale)
{
	labelScale = scale;
}

void DiveCartesianAxis::setMaximum(double maximum)
{
	if (IS_FP_SAME(max, maximum))
		return;
	max = maximum;
	emit maxChanged();
}

void DiveCartesianAxis::setMinimum(double minimum)
{
	if (IS_FP_SAME(min, minimum))
		return;
	min = minimum;
}

void DiveCartesianAxis::setTextColor(const QColor &color)
{
	textColor = color;
}

DiveCartesianAxis::DiveCartesianAxis() : QObject(),
	QGraphicsLineItem(),
	unitSystem(0),
	orientation(LeftToRight),
	min(0),
	max(0),
	interval(1),
	tick_size(0),
	labelScale(1.0),
	textVisibility(true),
	lineVisibility(true),
	line_size(1)
{
	setPen(gridPen());
}

DiveCartesianAxis::~DiveCartesianAxis()
{
}

void DiveCartesianAxis::setLineSize(qreal lineSize)
{
	line_size = lineSize;
}

void DiveCartesianAxis::setOrientation(Orientation o)
{
	orientation = o;
}

QColor DiveCartesianAxis::colorForValue(double value)
{
	return QColor(Qt::black);
}

void DiveCartesianAxis::setTextVisible(bool arg1)
{
	if (textVisibility == arg1) {
		return;
	}
	textVisibility = arg1;
	Q_FOREACH(DiveTextItem * item, labels) {
		item->setVisible(textVisibility);
	}
}

void DiveCartesianAxis::setLinesVisible(bool arg1)
{
	if (lineVisibility == arg1) {
		return;
	}
	lineVisibility = arg1;
	Q_FOREACH(DiveLineItem * item, lines) {
		item->setVisible(lineVisibility);
	}
}

template <typename T>
void emptyList(QList<T *> &list, double steps)
{
	if (!list.isEmpty() && list.size() > steps) {
		while (list.size() > steps) {
			T *removedItem = list.takeLast();
			Animations::animDelete(removedItem);
		}
	}
}

void DiveCartesianAxis::updateTicks()
{
	if (!scene())
		return;
	QLineF m = line();
	// unused so far:
	// QGraphicsView *view = scene()->views().first();
	double steps = (max - min) / interval;
	double currValueText = min;
	double currValueLine = min;

	if (steps < 1)
		return;

	emptyList(labels, steps);
	emptyList(lines, steps);

	// Move the remaining Ticks / Text to it's corerct position
	// Regartind the possibly new values for the Axis
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
	} else if (orientation == RightToLeft) {
		begin = m.x2();
		stepSize = (m.x2() - m.x1());
	}
	stepSize = stepSize / steps;

	for (int i = 0, count = labels.size(); i < count; i++, currValueText += interval) {
		qreal childPos = (orientation == TopToBottom || orientation == LeftToRight) ?
				     begin + i * stepSize :
				     begin - i * stepSize;

		labels[i]->setText(textForValue(currValueText));
		if (orientation == LeftToRight || orientation == RightToLeft) {
			labels[i]->animateMoveTo(childPos, m.y1() + tick_size);
		} else {
			labels[i]->animateMoveTo(m.x1() - tick_size, childPos);
		}
	}

	for (int i = 0, count = lines.size(); i < count; i++, currValueLine += interval) {
		qreal childPos = (orientation == TopToBottom || orientation == LeftToRight) ?
				     begin + i * stepSize :
				     begin - i * stepSize;

		if (orientation == LeftToRight || orientation == RightToLeft) {
			lines[i]->animateMoveTo(childPos, m.y1());
		} else {
			lines[i]->animateMoveTo(m.x1(), childPos);
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
		label->setBrush(QBrush(textColor));
		label->setBrush(colorForValue(currValueText));
		label->setScale(fontLabelScale());
		label->setZValue(1);
		labels.push_back(label);
		if (orientation == RightToLeft || orientation == LeftToRight) {
			label->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
			label->setPos(scene()->sceneRect().width() + 10, m.y1() + tick_size); // position it outside of the scene);
			label->animateMoveTo(childPos, m.y1() + tick_size);
		} else {
			label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
			label->setPos(m.x1() - tick_size, scene()->sceneRect().height() + 10);
			label->animateMoveTo(m.x1() - tick_size, childPos);
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
		QPen pen;
		pen.setBrush(getColor(TIME_GRID));
		pen.setCosmetic(true);
		pen.setWidthF(2);
		line->setPen(pen);
		line->setZValue(0);
		lines.push_back(line);
		if (orientation == RightToLeft || orientation == LeftToRight) {
			line->setLine(0, -line_size, 0, 0);
			line->animateMoveTo(childPos, m.y1());
		} else {
			QPointF p1 = mapFromScene(3, 0);
			QPointF p2 = mapFromScene(line_size, 0);
			line->setLine(p1.x(), 0, p2.x(), 0);
			line->animateMoveTo(m.x1(), childPos);
		}
	}

	Q_FOREACH(DiveTextItem * item, labels)
		item->setVisible(textVisibility);
	Q_FOREACH(DiveLineItem * item, lines)
		item->setVisible(lineVisibility);
}

void DiveCartesianAxis::animateChangeLine(const QLineF &newLine)
{
	setLine(newLine);
	updateTicks();
	sizeChanged();
}

QString DiveCartesianAxis::textForValue(double value)
{
	return QString::number(value);
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
	QLineF m = line();
	QPointF relativePosition = p;
	relativePosition -= pos(); // normalize p based on the axis' offset on screen

	double retValue = (orientation == LeftToRight || orientation == RightToLeft) ?
			      max * (relativePosition.x() - m.x1()) / (m.x2() - m.x1()) :
			      max * (relativePosition.y() - m.y1()) / (m.y2() - m.y1());
	return retValue;
}

qreal DiveCartesianAxis::posAtValue(qreal value)
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

qreal DiveCartesianAxis::percentAt(const QPointF &p)
{
	qreal value = valueAt(p);
	double size = max - min;
	double percent = value / size;
	return percent;
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
	QPen defaultPen(color);
	defaultPen.setJoinStyle(Qt::RoundJoin);
	defaultPen.setCapStyle(Qt::RoundCap);
	defaultPen.setWidth(2);
	defaultPen.setCosmetic(true);
	setPen(defaultPen);
}

QString DepthAxis::textForValue(double value)
{
	if (value == 0)
		return QString();
	return get_depth_string(value, false, false);
}

QColor DepthAxis::colorForValue(double value)
{
	Q_UNUSED(value);
	return QColor(Qt::red);
}

static bool isPPGraphEnabled()
{
	QSettings s;

	s.beginGroup("TecDetails");
	return s.value("phegraph").toBool() || s.value("po2graph").toBool() || s.value("pn2graph").toBool();
}

DepthAxis::DepthAxis() : showWithPPGraph(false)
{
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(settingsChanged()));

	// force the correct size of the line.
	showWithPPGraph = !isPPGraphEnabled();
	settingsChanged();
}

void DepthAxis::settingsChanged()
{
	// 	bool ppGraph = isPPGraphEnabled();
	// 	if ( ppGraph == showWithPPGraph){
	// 		return;
	// 	}
	//
	// 	if (ppGraph) {
	// 		animateChangeLine(shrinkedLine);
	// 	} else {
	// 		animateChangeLine(expandedLine);
	// 	}
	// 	showWithPPGraph = ppGraph;
}

QColor TimeAxis::colorForValue(double value)
{
	Q_UNUSED(value);
	return QColor(Qt::blue);
}

QString TimeAxis::textForValue(double value)
{
	int nr = value / 60;
	if (maximum() < 600)
		return QString("%1:%2").arg(nr).arg((int)value % 60, 2, 10, QChar('0'));
	return QString::number(nr);
}

void TimeAxis::updateTicks()
{
	DiveCartesianAxis::updateTicks();
	if (maximum() > 600) {
		for (int i = 0; i < labels.count(); i++) {
			labels[i]->setVisible(i % 2);
		}
	}
}

QString TemperatureAxis::textForValue(double value)
{
	return QString::number(mkelvin_to_C((int)value));
}

PartialGasPressureAxis::PartialGasPressureAxis()
{
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(preferencesChanged()));
}

void PartialGasPressureAxis::setModel(DivePlotDataModel *m)
{
	model = m;
	connect(model, SIGNAL(dataChanged(QModelIndex, QModelIndex)), this, SLOT(preferencesChanged()));
	preferencesChanged();
}

void PartialGasPressureAxis::preferencesChanged()
{
	QSettings s;
	s.beginGroup("TecDetails");
	bool showPhe = s.value("phegraph").toBool();
	bool showPn2 = s.value("pn2graph").toBool();
	bool showPo2 = s.value("po2graph").toBool();
	setVisible(showPhe || showPn2 || showPo2);
	if (!model->rowCount())
		return;

	double max = showPhe ? model->pheMax() : -1;
	if (showPn2 && model->pn2Max() > max)
		max = model->pn2Max();
	if (showPo2 && model->po2Max() > max)
		max = model->po2Max();

	qreal pp = floor(max * 10.0) / 10.0 + 0.2;
	if (IS_FP_SAME(maximum(), pp))
		return;

	setMaximum(pp);
	setTickInterval(pp > 4 ? 0.5 : 0.25);
	updateTicks();
}
