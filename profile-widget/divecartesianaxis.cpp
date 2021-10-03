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
	changed = !IS_FP_SAME(max, maximum) || !IS_FP_SAME(min, minimum);
	dataMin = min = minimum;
	dataMax = max = maximum;
}

DiveCartesianAxis::DiveCartesianAxis(Position position, int integralDigits, int fractionalDigits, color_index_t gridColor,
				     bool textVisible, bool linesVisible,
				     double dpr, double labelScale, bool printMode, bool isGrayscale, ProfileScene &scene) :
	printMode(printMode),
	position(position),
	fractionalDigits(fractionalDigits),
	gridColor(gridColor),
	scene(scene),
	orientation(LeftToRight),
	min(0),
	max(0),
	textVisibility(textVisible),
	lineVisibility(linesVisible),
	labelScale(labelScale),
	changed(true),
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

	/* Use the label to estimate size of the labels.
	 * Round up, because non-integers tend to give abysmal rendering.
	 */
	QFont fnt = DiveTextItem::getFont(dpr, labelScale);
	double outlineSpace = DiveTextItem::outlineSpace(dpr);
	QFontMetrics fm(fnt);
	labelWidth = ceil(fm.size(Qt::TextSingleLine, label).width() + outlineSpace);
	labelHeight = ceil(fm.height() + outlineSpace);
}

DiveCartesianAxis::~DiveCartesianAxis()
{
}

void DiveCartesianAxis::setOrientation(Orientation o)
{
	orientation = o;
	changed = true;
}

void DiveCartesianAxis::setTransform(double a, double b)
{
	transform.a = a;
	transform.b = b;
	changed = true;
}

QColor DiveCartesianAxis::colorForValue(double) const
{
	return QColor(Qt::black);
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

static double sensibleInterval(double inc, int decimals)
{
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
	// Do "nice" increments of the leading digit: 1, 2, 4, 5.
	if (inc_int > 5)
		inc_int = 10;
	if (inc_int == 3)
		inc_int = 4;
	inc = inc_int * digits_factor;

	return inc;
}

void DiveCartesianAxis::updateTicks(int animSpeed)
{
	if (!changed && !printMode)
		return;

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
	intervalDisplay = sensibleInterval(intervalDisplay, fractionalDigits);

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

	stepValue = position == Position::Bottom ?
		intervalDisplay / transform.a :		// special case for time axis.
		numTicks > 1 ? (max - min) / (numTicks - 1) : 0;
	double stepScreen = stepValue * size / (max - min);

	// Move the remaining grid lines / labels to their correct positions
	// regarding the possible new values for the axis
	double firstPosScreen;
	if (orientation == TopToBottom) {
		firstPosScreen = m.y1();
	} else if (orientation == BottomToTop) {
		firstPosScreen = m.y2();
	} else if (orientation == LeftToRight) {
		firstPosScreen = m.x1();
	} else /* if (orientation == RightToLeft) */ {
		firstPosScreen = m.x2();
	}

	if (textVisibility)
		updateLabels(numTicks, firstPosScreen, firstValue, stepScreen, stepValue, animSpeed);
	if (lineVisibility)
		updateLines(numTicks, firstPosScreen, stepScreen, animSpeed);
	changed = false;
}

void DiveCartesianAxis::updateLabels(int numTicks, double firstPosScreen, double firstValue, double stepScreen, double stepValue, int animSpeed)
{
	for (int i = 0, count = labels.size(); i < count; i++, firstValue += stepValue) {
		double childPos = (orientation == TopToBottom || orientation == LeftToRight) ?
					 firstPosScreen + i * stepScreen :
					 firstPosScreen - i * stepScreen;

		labels[i]->set(textForValue(firstValue), colorForValue(firstValue));
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
		double childPos;
		if (orientation == TopToBottom || orientation == LeftToRight) {
			childPos = firstPosScreen + i * stepScreen;
		} else {
			childPos = firstPosScreen - i * stepScreen;
		}
		int alignFlags = position == Position::Bottom ? Qt::AlignTop | Qt::AlignHCenter :
				 position == Position::Left   ? Qt::AlignVCenter | Qt::AlignLeft:
								Qt::AlignVCenter | Qt::AlignRight;
		DiveTextItem *label = new DiveTextItem(dpr, labelScale, alignFlags, this);
		label->set(textForValue(firstValue), colorForValue(firstValue));
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
		double childPos = (orientation == TopToBottom || orientation == LeftToRight) ?
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
		double childPos;
		if (orientation == TopToBottom || orientation == LeftToRight) {
			childPos = firstPosScreen + i * stepScreen;
		} else {
			childPos = firstPosScreen - i * stepScreen;
		}
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
	changed = true;
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

QColor DepthAxis::colorForValue(double) const
{
	return QColor(Qt::red);
}

QColor TimeAxis::colorForValue(double) const
{
	return QColor(Qt::blue);
}
