// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/divecartesianaxis.h"
#include "profile-widget/divetextitem.h"
#include "core/qthelper.h"
#include "core/subsurface-string.h"
#include "qt-models/diveplotdatamodel.h"
#include "profile-widget/animationfunctions.h"
#include "profile-widget/divelineitem.h"
#include "profile-widget/profilescene.h"

static const double labelSpaceHorizontal = 2.0; // space between label and ticks
static const double labelSpaceVertical = 2.0; // space between label and ticks

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

void DiveCartesianAxis::setMaximum(double maximum)
{
	if (IS_FP_SAME(max, maximum))
		return;
	max = maximum;
	changed = true;
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

DiveCartesianAxis::DiveCartesianAxis(Position position, color_index_t gridColor, double dpr, bool printMode, ProfileScene &scene) :
	printMode(printMode),
	position(position),
	gridColor(gridColor),
	scene(scene),
	orientation(LeftToRight),
	min(0),
	max(0),
	interval(1),
	textVisibility(true),
	lineVisibility(true),
	labelScale(1.0),
	changed(true),
	dpr(dpr)
{
	setPen(gridPen());
}

DiveCartesianAxis::~DiveCartesianAxis()
{
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

double DiveCartesianAxis::textWidth(const QString &s) const
{
	QFont fnt = DiveTextItem::getFont(dpr, labelScale);
	QFontMetrics fm(fnt);
	return fm.size(Qt::TextSingleLine, s).width() + labelSpaceHorizontal * dpr;
}

double DiveCartesianAxis::width() const
{
	return textWidth("999");
}

double DiveCartesianAxis::height() const
{
	QFont fnt = DiveTextItem::getFont(dpr, labelScale);
	QFontMetrics fm(fnt);
	return fm.height() + labelSpaceVertical * dpr;
}

void DiveCartesianAxis::updateTicks(int animSpeed)
{
	if (!changed && !printMode)
		return;
	QLineF m = line();
	double stepsInRange = (max - min) / interval;
	int steps = (int)stepsInRange;
	double currValueText = min;
	double currValueLine = min;

	if (steps < 1)
		return;

	emptyList(labels, steps, animSpeed);
	emptyList(lines, steps, animSpeed);

	// Move the remaining grid lines / labels to their correct positions
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
		double childPos = (orientation == TopToBottom || orientation == LeftToRight) ?
					 begin + i * stepSize :
					 begin - i * stepSize;

		labels[i]->set(textForValue(currValueText), colorForValue(currValueText));
		switch (position) {
		default:
		case Position::Bottom:
			Animations::moveTo(labels[i], animSpeed, childPos, rect.bottom() + labelSpaceVertical * dpr);
			break;
		case Position::Left:
			Animations::moveTo(labels[i], animSpeed, rect.left() - labelSpaceHorizontal * dpr, childPos);
			break;
		case Position::Right:
			Animations::moveTo(labels[i], animSpeed, rect.right() + labelSpaceHorizontal * dpr, childPos);
			break;
		}
	}

	for (int i = 0, count = lines.size(); i < count; i++, currValueLine += interval) {
		double childPos = (orientation == TopToBottom || orientation == LeftToRight) ?
					 begin + i * stepSize :
					 begin - i * stepSize;

		if (position == Position::Bottom) {
			// Fix size in case the scene changed
			QLineF old = lines[i]->line();
			lines[i]->setLine(old.x1(), old.y1(), old.x1(), old.y1() + rect.height());
			Animations::moveTo(lines[i], animSpeed, childPos, rect.top());
		} else {
			// Fix size in case the scene changed
			QLineF old = lines[i]->line();
			lines[i]->setLine(old.x1(), old.y1(), old.x1() + rect.width(), old.y1());
			Animations::moveTo(lines[i], animSpeed, rect.left(), childPos);
		}
	}

	// Add the rest of the needed labels.
	for (int i = labels.size(); i < steps; i++, currValueText += interval) {
		double childPos;
		if (orientation == TopToBottom || orientation == LeftToRight) {
			childPos = begin + i * stepSize;
		} else {
			childPos = begin - i * stepSize;
		}
		int alignFlags = position == Position::Bottom ? Qt::AlignTop | Qt::AlignHCenter :
				 position == Position::Left   ? Qt::AlignVCenter | Qt::AlignLeft:
								Qt::AlignVCenter | Qt::AlignRight;
		DiveTextItem *label = new DiveTextItem(dpr, labelScale, alignFlags, this);
		label->set(textForValue(currValueText), colorForValue(currValueText));
		label->setZValue(1);
		labels.push_back(label);
		switch (position) {
		default:
		case Position::Bottom:
			label->setPos(scene.sceneRect().width() + 10, rect.bottom() + labelSpaceVertical * dpr); // position it outside of the scene;
			Animations::moveTo(labels[i], animSpeed, childPos, rect.bottom() + labelSpaceVertical * dpr);
			break;
		case Position::Left:
			label->setPos(rect.left() - labelSpaceHorizontal * dpr, scene.sceneRect().height() + 10);
			Animations::moveTo(labels[i], animSpeed, rect.left() - labelSpaceHorizontal * dpr, childPos);
			break;
		case Position::Right:
			label->setPos(rect.right() + labelSpaceHorizontal * dpr, scene.sceneRect().height() + 10);
			Animations::moveTo(labels[i], animSpeed, rect.right() + labelSpaceHorizontal * dpr, childPos);
			break;
		}
	}

	// Add the rest of the needed grid lines.
	for (int i = lines.size(); i < steps; i++, currValueText += interval) {
		double childPos;
		if (orientation == TopToBottom || orientation == LeftToRight) {
			childPos = begin + i * stepSize;
		} else {
			childPos = begin - i * stepSize;
		}
		DiveLineItem *line = new DiveLineItem(this);
		QPen pen = gridPen();
		pen.setBrush(getColor(gridColor));
		line->setPen(pen);
		line->setZValue(0);
		lines.push_back(line);
		if (position == Position::Bottom) {
			line->setLine(0.0, 0.0, 0.0, rect.height());
			line->setPos(scene.sceneRect().width() + 10, rect.top()); // position it outside of the scene);
			Animations::moveTo(line, animSpeed, childPos, rect.top());
		} else {
			line->setLine(0.0, 0.0, rect.width(), 0.0);
			line->setPos(rect.left(), scene.sceneRect().height() + 10);
			Animations::moveTo(line, animSpeed, rect.left(), childPos);
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

void DiveCartesianAxis::animateChangeLine(const QRectF &rectIn, int animSpeed)
{
	rect = rectIn;
	switch (position) {
		case Position::Left:
			setLine(QLineF(rect.topLeft(), rect.bottomLeft()));
			break;
		case Position::Right:
			setLine(QLineF(rect.topRight(), rect.bottomRight()));
			break;
		case Position::Bottom:
		default:
			setLine(QLineF(rect.bottomLeft(), rect.bottomRight()));
			break;
	}
	updateTicks(animSpeed);
	sizeChanged();
}

QString DiveCartesianAxis::textForValue(double value) const
{
	return QString("%L1").arg(value, 0, 'g', 4);
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

std::pair<double, double> DiveCartesianAxis::screenMinMax() const
{
	return position == Position::Bottom ? std::make_pair(rect.left(), rect.right())
					    : std::make_pair(rect.top(), rect.bottom());
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

DepthAxis::DepthAxis(Position position, color_index_t gridColor, double dpr, bool printMode, ProfileScene &scene) :
	DiveCartesianAxis(position, gridColor, dpr, printMode, scene)
{
	changed = true;
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

// TODO: replace by real dynamic axis - this is just weird.
void TimeAxis::updateTicks(int animSpeed)
{
	DiveCartesianAxis::updateTicks(animSpeed);
	if (maximum() > 600) {
		for (int i = 0; i < labels.count(); i++) {
			labels[i]->setVisible(i % 2);
		}
	}
}

QString TemperatureAxis::textForValue(double value) const
{
	return QString::number(mkelvin_to_C((int)value));
}

PartialGasPressureAxis::PartialGasPressureAxis(const DivePlotDataModel &model, Position position, color_index_t gridColor,
					       double dpr, bool printMode, ProfileScene &scene) :
	DiveCartesianAxis(position, gridColor, dpr, printMode, scene),
	model(model)
{
}

void PartialGasPressureAxis::update(int animSpeed)
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
	updateTicks(animSpeed);
}

double PartialGasPressureAxis::width() const
{
	return textWidth(textForValue(0.99));
}
