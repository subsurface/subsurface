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

static QPen gridPen(){
	QPen pen;
	pen.setColor(getColor(TIME_GRID));
	pen.setWidth(2);
	pen.setCosmetic(true);
	return pen;
}

void DiveCartesianAxis::setup(double minimum, double maximum, double interval, DiveCartesianAxis::Orientation o, qreal tick_size, const QPointF& pos)
{
	setMinimum(minimum);
	setMaximum(maximum);
	setTickInterval(interval);
	setOrientation(o);
	setTickSize(tick_size);
	setPos(pos);
}

double DiveCartesianAxis::tickInterval() const
{
	return interval;
}

double DiveCartesianAxis::tickSize() const
{
	return tick_size;
}

void DiveCartesianAxis::setMaximum(double maximum)
{
	if (max == maximum)
		return;
	max = maximum;
	emit maxChanged();
}

void DiveCartesianAxis::setMinimum(double minimum)
{
	if (min == minimum)
		return;
	min = minimum;
}

void DiveCartesianAxis::setTextColor(const QColor& color)
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
	tick_size(0)
{
	setPen(gridPen());
}

DiveCartesianAxis::~DiveCartesianAxis()
{

}

void DiveCartesianAxis::setOrientation(Orientation o)
{
	orientation = o;
}

QColor DiveCartesianAxis::colorForValue(double value)
{
	return QColor(Qt::black);
}

void DiveCartesianAxis::updateTicks()
{
	if (!scene())
		return;
	QLineF m = line();
	// unused so far:
	// QGraphicsView *view = scene()->views().first();
	double steps = (max - min) / interval;
	double currValue = min;

	if (steps < 1)
		return;

	if (!labels.isEmpty() && labels.size() > steps) {
		while (labels.size() > steps) {
				DiveTextItem *removedText = labels.takeLast();
				Animations::animDelete(removedText);
		}
	}
	// Move the remaining Ticks / Text to it's corerct position
	// Regartind the possibly new values for the Axis
	qreal begin, stepSize;
	if (orientation == TopToBottom) {
		begin = m.y1();
		stepSize = (m.y2() - m.y1());
	} else if (orientation == BottomToTop) {
		begin = m.y2();
		stepSize = (m.y2() - m.y1());
	} else if (orientation == LeftToRight ) {
		begin = m.x1();
		stepSize = (m.x2() - m.x1());
	} else if (orientation == RightToLeft) {
		begin = m.x2();
		stepSize = (m.x2() - m.x1());
	}
	stepSize = stepSize / steps;

	for (int i = 0, count = labels.size(); i < count; i++, currValue += interval) {
		qreal childPos;
		if (orientation == TopToBottom || orientation == LeftToRight) {
			childPos = begin + i * stepSize;
		} else {
			childPos = begin - i * stepSize;
		}

		labels[i]->setText(textForValue(currValue));
		if ( orientation == LeftToRight || orientation == RightToLeft) {
			labels[i]->animateMoveTo(childPos, m.y1() + tick_size);
		} else {
			labels[i]->animateMoveTo(m.x1() - tick_size, childPos);
		}
	}

	// Add's the rest of the needed Ticks / Text.
	for (int i = labels.size(); i < steps; i++,  currValue += interval) {
		qreal childPos;
		if (orientation == TopToBottom || orientation == LeftToRight) {
		childPos = begin + i * stepSize;
		} else {
			childPos = begin - i * stepSize;
		}
		DiveTextItem *label = new DiveTextItem(this);
		label->setText(textForValue(currValue));
		label->setBrush(QBrush(textColor));
		label->setBrush(colorForValue(currValue));
		labels.push_back(label);
		if (orientation == RightToLeft || orientation == LeftToRight) {
			label->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
			label->setPos(scene()->sceneRect().width() + 10, m.y1() + tick_size); // position it outside of the scene);
			label->animateMoveTo(childPos, m.y1() + tick_size);
		} else {
			label->setAlignment(Qt::AlignVCenter| Qt::AlignLeft);
			label->setPos(m.x1() - tick_size, scene()->sceneRect().height() + 10);
			label->animateMoveTo(m.x1() - tick_size, childPos);
		}
	}
}

void DiveCartesianAxis::animateChangeLine(const QLineF& newLine)
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

qreal DiveCartesianAxis::valueAt(const QPointF& p)
{
	QLineF m = line();
	double retValue =  (orientation == LeftToRight || orientation == RightToLeft) ?
				max * (p.x() - m.x1()) / (m.x2() - m.x1()) :
				max * (p.y() - m.y1()) / (m.y2() - m.y1());
	return retValue;
}

qreal DiveCartesianAxis::posAtValue(qreal value)
{
	QLineF m = line();
	QPointF p = pos();

	double size = max - min;
	// unused for now:
	// double distanceFromOrigin = value - min;
	double percent =  IS_FP_SAME(min,max) ? 0.0 : (value - min) / size;


	double realSize = orientation == LeftToRight || orientation == RightToLeft?
				m.x2() - m.x1() :
				m.y2() - m.y1();

	// Inverted axis, just invert the percentage.
	if (orientation == RightToLeft || orientation == BottomToTop)
		percent = 1 - percent;

	double retValue = realSize * percent;
	double adjusted =
		orientation == LeftToRight ?  retValue + m.x1() + p.x() :
		orientation == RightToLeft ?  retValue + m.x1() + p.x() :
		orientation == TopToBottom ?  retValue + m.y1() + p.y() :
		/* entation == BottomToTop */ retValue + m.y1() + p.y() ;
	return adjusted;
}

qreal DiveCartesianAxis::percentAt(const QPointF& p)
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

void DiveCartesianAxis::setColor(const QColor& color)
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
	if (maximum() < 600 )
		return QString("%1:%2").arg(nr).arg( (int)value%60, 2, 10, QChar('0'));
	return  QString::number(nr);
}

void TimeAxis::updateTicks()
{
	DiveCartesianAxis::updateTicks();
	if (maximum() > 600){
		for(int i = 0; i < labels.count(); i++){
			labels[i]->setVisible(i % 2);
		}
	}
}

QString TemperatureAxis::textForValue(double value)
{
	return QString::number(mkelvin_to_C( (int) value));
}


void DiveCartesianPlane::setLeftAxis(DiveCartesianAxis* axis)
{
	leftAxis = axis;
	connect(leftAxis, SIGNAL(maxChanged()), this, SLOT(setup()));
	if (bottomAxis)
		setup();
}

void DiveCartesianPlane::setBottomAxis(DiveCartesianAxis* axis)
{
	bottomAxis = axis;
	connect(bottomAxis, SIGNAL(maxChanged()), this, SLOT(setup()));
	if (leftAxis)
		setup();
}

QLineF DiveCartesianPlane::horizontalLine() const
{
	return (bottomAxis) ? bottomAxis->line() : QLineF();
}

void DiveCartesianPlane::setHorizontalLine(QLineF line)
{
	if ( horizontalSize == line.length())
		return;
	horizontalSize = line.length();
	setup();
}

void DiveCartesianPlane::setVerticalLine(QLineF line)
{
	if (verticalSize == line.length())
		return;
	verticalSize = line.length();
	setup();
}

QLineF DiveCartesianPlane::verticalLine() const
{
	return (leftAxis) ? leftAxis->line() : QLineF();
}

void DiveCartesianPlane::setup()
{
	if (!leftAxis || !bottomAxis || !scene())
		return;

	setPen(gridPen());
// This creates a Grid around the axis, creating the cartesian plane.
	const int top = leftAxis->posAtValue(leftAxis->minimum());
	// unused for now:
	// const int bottom = leftAxis->posAtValue(leftAxis->maximum());
	const int left = bottomAxis->posAtValue(bottomAxis->minimum());
	// unused for now:
	// const int right = bottomAxis->posAtValue(bottomAxis->maximum());

	setRect(0, 0, horizontalSize, verticalSize);
	setPos(left, top);

	qDeleteAll(horizontalLines);
	qDeleteAll(verticalLines);
	horizontalLines.clear();
	verticalLines.clear();

	// DEPTH is M_OR_FEET(10,30), Minutes are 600, per line.
	for (int i = leftAxis->minimum(), max = leftAxis->maximum(); i < max; i += M_OR_FT(10,30)) {
		DiveLineItem *line = new DiveLineItem(this);
		line->setLine(0, 0, horizontalSize, 0);
		line->setPos(0,leftAxis->posAtValue(i)-top);
		line->setZValue(-1);
		line->setPen(gridPen());
		horizontalLines.push_back(line);
	}

	for (int i = bottomAxis->minimum(), max = bottomAxis->maximum(); i < max; i += 600) { // increments by 10 minutes.
		DiveLineItem *line = new DiveLineItem(this);
		line->setLine(0, 0, 0, verticalSize);
		line->setPos(bottomAxis->posAtValue(i)-left, 0);
		line->setZValue(-1);
		line->setPen(gridPen());
		verticalLines.push_back(line);
	}
}

PartialGasPressureAxis::PartialGasPressureAxis()
{
	connect(PreferencesDialog::instance(), SIGNAL(settingsChanged()), this, SLOT(preferencesChanged()));
}

void PartialGasPressureAxis::setModel(DivePlotDataModel* m)
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
	if( showPo2 && model->po2Max() > max)
		max = model->po2Max();

	qreal pp = floor(max * 10.0) / 10.0 + 0.2;
	if (maximum() == pp)
		return;

	setMaximum(pp);
	setTickInterval( pp > 4 ? 0.5 : 0.25 );
	updateTicks();
}
