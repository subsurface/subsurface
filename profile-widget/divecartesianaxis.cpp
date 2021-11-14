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

void DiveCartesianAxis::setBounds(double minimum, double maximum)
{
	dataMin = min = minimum;
	dataMax = max = maximum;
}

DiveCartesianAxis::DiveCartesianAxis(Position position, bool inverted, int integralDigits, int fractionalDigits, color_index_t gridColor,
				     QColor textColor, bool textVisible, bool linesVisible,
				     double dpr, double labelScale, bool printMode, bool isGrayscale, ProfileScene &scene) :
	printMode(printMode),
	position(position),
	inverted(inverted),
	fractionalDigits(fractionalDigits),
	textColor(textColor),
	scene(scene),
	min(0),
	max(0),
	textVisibility(textVisible),
	lineVisibility(linesVisible),
	labelScale(labelScale),
	dpr(dpr),
	transform({1.0, 0.0})
{
	QPen pen;
	pen.setColor(getColor(TIME_GRID, isGrayscale));
	/* cosmetic width() == 0 for lines in printMode
	 * having setCosmetic(true) and width() > 0 does not work when
	 * printing on OSX and Linux */
	pen.setWidth(DiveCartesianAxis::printMode ? 0 : 2);
	pen.setCosmetic(true);
	setPen(pen);

	pen.setBrush(getColor(gridColor, isGrayscale));
	gridPen = pen;

	/* Create the longest expected label, e.g. 999.99. */
	QString label;
	label.reserve(integralDigits + fractionalDigits + 1);
	for (int i = 0; i < integralDigits; ++i)
		label.append('9');
	if (fractionalDigits > 0) {
		label.append('.');
		for (int i = 0; i < fractionalDigits; ++i)
			label.append('9');
	}

	std::tie(labelWidth, labelHeight) = DiveTextItem::getLabelSize(dpr, labelScale, label);
}

DiveCartesianAxis::~DiveCartesianAxis()
{
}

void DiveCartesianAxis::setTransform(double a, double b)
{
	transform.a = a;
	transform.b = b;
}

template <typename T>
void emptyList(QList<T *> &list, int steps, int speed)
{
	while (list.size() > steps) {
		T *removedItem = list.takeLast();
		Animations::animDelete(removedItem, speed);
	}
}

double DiveCartesianAxis::width() const
{
	return labelWidth + labelSpaceHorizontal * dpr;
}

double DiveCartesianAxis::height() const
{
	return labelHeight + labelSpaceVertical * dpr;
}

double DiveCartesianAxis::horizontalOverhang() const
{
	return labelWidth / 2.0;
}

int DiveCartesianAxis::getMinLabelDistance(const DiveCartesianAxis &timeAxis) const
{
	// For the plot not being to crowded we want at least two
	// labels to fit between each pair of displayed labels.
	// May need some optimization.
	QLineF m = timeAxis.line();
	double interval = labelWidth * 3.0 * (timeAxis.maximum() - timeAxis.minimum()) / (m.x2() - m.x1());
	return int(ceil(interval));
}

static double sensibleInterval(double inc, int decimals, bool is_time_axis)
{
	if (is_time_axis && inc < 60.0) {
		// for time axes and less than one hour increments, round to
		// 1, 2, 3, 4, 5, 6  or 12 parts of an hour or a minute
		// (that is 60, 30, 20, 15, 12, 10 or 5 min/sec).
		bool fraction_of_hour = inc > 1.0;
		if (fraction_of_hour)
			inc /= 60.0;
		inc = inc <= 1.0 / 12.0 ? 1.0 / 12.0 :
		      inc <= 1.0 / 6.0  ? 1.0 / 6.0 :
					  1.0 / floor(1.0/inc);
		if (fraction_of_hour)
			inc *= 60.0;
		return inc;
	}

	// Use full decimal increments
	double digits = floor(log10(inc));
	int digits_int = lrint(digits);

	// Don't do increments smaller than the displayed decimals.
	if (digits_int < -decimals) {
		digits_int = -decimals;
		digits = static_cast<double>(digits_int);
	}

	double digits_factor = pow(10.0, digits);
	int inc_int = std::max((int)ceil(inc / digits_factor), 1);
	// Do "nice" increments of the leading digit. In general: 1, 2, 4, 5.
	if (inc_int > 5)
		inc_int = 10;
	if (inc_int == 3)
		inc_int = 4;
	inc = inc_int * digits_factor;

	return inc;
}

void DiveCartesianAxis::updateTicks(int animSpeed)
{
	if (dataMax - dataMin < 1e-5)
		return;

	if (!textVisibility && !lineVisibility)
		return; // Nothing to display...

	// Guess the number of tick marks.
	QLineF m = line();
	double spaceNeeded = position == Position::Bottom ? labelWidth * 3.0 / 2.0
							  : labelHeight * 2.0;
	double size = position == Position::Bottom ? fabs(m.x2() - m.x1())
						   : fabs(m.y2() - m.y1());
	int numTicks = lrint(size / spaceNeeded);

	numTicks = std::clamp(numTicks, 2, 50);
	double stepValue = (dataMax - dataMin) / numTicks;

	// Round the interval to a sensible size in display units
	double intervalDisplay = stepValue * transform.a;
	intervalDisplay = sensibleInterval(intervalDisplay, fractionalDigits, position == Position::Bottom);

	// Choose full multiples of the interval as minumum and maximum values
	double minDisplay = transform.to(dataMin);
	double maxDisplay = transform.to(dataMax);

	// The time axis is special: use the full width in that case.
	// Other axes round to the next "nice" number
	double firstDisplay, lastDisplay;
	double firstValue;
	if (position == Position::Bottom) {
		firstDisplay = ceil(minDisplay / intervalDisplay * (1.0 - 1e-5)) * intervalDisplay;
		lastDisplay = floor(maxDisplay / intervalDisplay * (1.0 + 1e-5)) * intervalDisplay;
		firstValue = transform.from(firstDisplay);
	} else {
		firstDisplay = floor(minDisplay / intervalDisplay * (1.0 + 1e-5)) * intervalDisplay;
		lastDisplay = ceil(maxDisplay / intervalDisplay * (1.0 - 1e-5)) * intervalDisplay;
		min = transform.from(firstDisplay);
		max = transform.from(lastDisplay);
		firstValue = min;
	}
	numTicks = lrint((lastDisplay - firstDisplay) / intervalDisplay) + 1;
	numTicks = std::max(numTicks, 0);

	emptyList(labels, numTicks, animSpeed);
	emptyList(lines, numTicks, animSpeed);
	if (numTicks == 0)
		return;

	double internalToScreen = size / (max - min);
	stepValue = position == Position::Bottom ?
		intervalDisplay / transform.a :		// special case for time axis.
		numTicks > 1 ? (max - min) / (numTicks - 1) : 0;
	double stepScreen = stepValue * internalToScreen;

	// The ticks of the time axis don't necessarily start at the beginning.
	double offsetScreen = position == Position::Bottom ?
		(firstValue - min) * internalToScreen : 0.0;

	// Move the remaining grid lines / labels to their correct positions
	// regarding the possible new values for the axis.
	double firstPosScreen = position == Position::Bottom ?
		(inverted ? m.x2() - offsetScreen : m.x1() + offsetScreen) :
		(inverted ? m.y1() + offsetScreen : m.y2() - offsetScreen);

	if (textVisibility)
		updateLabels(numTicks, firstPosScreen, firstValue, stepScreen, stepValue, animSpeed);
	if (lineVisibility)
		updateLines(numTicks, firstPosScreen, stepScreen, animSpeed);
}

void DiveCartesianAxis::updateLabels(int numTicks, double firstPosScreen, double firstValue, double stepScreen, double stepValue, int animSpeed)
{
	for (int i = 0, count = labels.size(); i < count; i++, firstValue += stepValue) {
		double childPos = ((position == Position::Bottom) != inverted) ?
					 firstPosScreen + i * stepScreen :
					 firstPosScreen - i * stepScreen;
		labels[i]->set(textForValue(firstValue), textColor);
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

	// Add the rest of the needed labels.
	for (int i = labels.size(); i < numTicks; i++, firstValue += stepValue) {
		double childPos = ((position == Position::Bottom) != inverted) ?
					firstPosScreen + i * stepScreen :
					firstPosScreen - i * stepScreen;
		int alignFlags = position == Position::Bottom ? Qt::AlignTop | Qt::AlignHCenter :
				 position == Position::Left   ? Qt::AlignVCenter | Qt::AlignLeft:
								Qt::AlignVCenter | Qt::AlignRight;
		DiveTextItem *label = new DiveTextItem(dpr, labelScale, alignFlags, this);
		label->set(textForValue(firstValue), textColor);
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
}

void DiveCartesianAxis::updateLines(int numTicks, double firstPosScreen, double stepScreen, int animSpeed)
{
	for (int i = 0, count = lines.size(); i < count; i++) {
		double childPos = ((position == Position::Bottom) != inverted) ?
					firstPosScreen + i * stepScreen :
					firstPosScreen - i * stepScreen;

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

	// Add the rest of the needed grid lines.
	for (int i = lines.size(); i < numTicks; i++) {
		double childPos = ((position == Position::Bottom) != inverted) ?
					firstPosScreen + i * stepScreen :
					firstPosScreen - i * stepScreen;
		DiveLineItem *line = new DiveLineItem(this);
		line->setPen(gridPen);
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
}

void DiveCartesianAxis::setPosition(const QRectF &rectIn)
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
}

double DiveCartesianAxis::Transform::to(double x) const
{
	return a*x + b;
}

double DiveCartesianAxis::Transform::from(double y) const
{
	return (y - b) / a;
}

QString DiveCartesianAxis::textForValue(double value) const
{
	if (position == Position::Bottom) {
		// The bottom axis is the time axis and that needs special treatment.
		int nr = lrint(value) / 60;
		if (maximum() - minimum() < 600.0)
			return QString("%1:%2").arg(nr).arg((int)value % 60, 2, 10, QChar('0'));
		return QString::number(nr);
	} else {
		return QStringLiteral("%L1").arg(transform.to(value), 0, 'f', fractionalDigits);
	}
}

qreal DiveCartesianAxis::valueAt(const QPointF &p) const
{
	QLineF m = line();
	QPointF relativePosition = p;
	relativePosition -= pos(); // normalize p based on the axis' offset on screen

	double fraction = position == Position::Bottom ?
		(relativePosition.x() - m.x1()) / (m.x2() - m.x1()) :
		(relativePosition.y() - m.y1()) / (m.y2() - m.y1());

	if ((position == Position::Bottom) == inverted)
		fraction = 1.0 - fraction;
	return fraction * (max - min) + min;
}

qreal DiveCartesianAxis::posAtValue(qreal value) const
{
	QLineF m = line();

	double screenFrom = position == Position::Bottom ? m.x1() : m.y1();
	double screenTo = position == Position::Bottom ? m.x2() : m.y2();
	if (IS_FP_SAME(min, max))
		return (screenFrom + screenTo) / 2.0;
	if ((position == Position::Bottom) == inverted)
		std::swap(screenFrom, screenTo);
	return (value - min) / (max - min) *
	       (screenTo - screenFrom) + screenFrom;
}

static std::pair<double, double> getLineFromTo(const QLineF &l, bool horizontal)
{
	if (horizontal)
		return std::make_pair(l.x1(), l.x2());
	else
		return std::make_pair(l.y1(), l.y2());
}

double DiveCartesianAxis::screenPosition(double pos) const
{
	if ((position == Position::Bottom) == inverted)
		pos = 1.0 - pos;

	auto [from, to] = getLineFromTo(line(), position == Position::Bottom);
	return (to - from) * pos + from;
}

double DiveCartesianAxis::pointInRange(double pos) const
{
	auto [from, to] = getLineFromTo(line(), position == Position::Bottom);
	if (from > to)
		std::swap(from, to);
	return pos >= from && pos <= to;
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
