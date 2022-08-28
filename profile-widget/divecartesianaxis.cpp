// SPDX-License-Identifier: GPL-2.0
#include "profile-widget/divecartesianaxis.h"
#include "profile-widget/divetextitem.h"
#include "core/profile.h"
#include "core/qthelper.h"
#include "core/subsurface-float.h"
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
	gridIsMultipleOfThree(false),
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

	// The axis is implemented by a line, which gives the position.
	// If we don't show labels or grid, we don't want to show the line,
	// as this gives a strange artifact. TODO: Don't derive from a
	// line object in the first place.
	setVisible(textVisible || linesVisible);
}

DiveCartesianAxis::~DiveCartesianAxis()
{
}

void DiveCartesianAxis::setTransform(double a, double b)
{
	transform.a = a;
	transform.b = b;
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

static double sensibleInterval(double inc, int decimals, bool is_time_axis, bool is_multiple_of_three)
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
	if (is_multiple_of_three)
	{
		// Do increments quantized to 3. In general: 1, 3, 6, 15
		if (inc_int > 6)
			inc_int = 15;
		else if (inc_int > 3)
			inc_int = 6;
		else if (inc_int == 2)
			inc_int = 3;
	} else {
		// Do "nice" increments of the leading digit. In general: 1, 2, 4, 5.
		if (inc_int > 5)
			inc_int = 10;
		if (inc_int == 3)
			inc_int = 4;
	}
	inc = inc_int * digits_factor;

	return inc;
}

void DiveCartesianAxis::updateTicks(int animSpeed)
{
	if (dataMax - dataMin < 1e-5)
		return;

	if (!textVisibility && !lineVisibility)
		return; // Nothing to display...

	// Remember the old range for animations.
	double dataMaxOld = dataMax;
	double dataMinOld = dataMin;

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
	intervalDisplay = sensibleInterval(intervalDisplay, fractionalDigits, position == Position::Bottom, gridIsMultipleOfThree);

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

	updateLabels(numTicks, firstPosScreen, firstValue, stepScreen, stepValue, animSpeed, dataMinOld, dataMaxOld);
}


QPointF DiveCartesianAxis::labelPos(double pos) const
{
	return position == Position::Bottom ? QPointF(pos, rect.bottom() + labelSpaceVertical * dpr) :
	       position == Position::Left   ? QPointF(rect.left() - labelSpaceHorizontal * dpr, pos) :
					      QPointF(rect.right() + labelSpaceHorizontal * dpr, pos);
}

QLineF DiveCartesianAxis::linePos(double pos) const
{
	return position == Position::Bottom ? QLineF(pos, rect.top(), pos, rect.bottom()) :
					      QLineF(rect.left(), pos, rect.right(), pos);
}

void DiveCartesianAxis::updateLabel(Label &label, double opacityEnd, double pos) const
{
	label.opacityStart = label.label ? label.label->opacity()
					 : label.line->opacity();
	label.opacityEnd = opacityEnd;
	if (label.label) {
		label.labelPosStart = label.label->pos();
		label.labelPosEnd = labelPos(pos);

		// For the time-axis, the format might change from "mm" to "mm:ss",
		// or vice versa. Currently, we don't animate that, i.e. it will
		// switch instantaneously.
		if (position == Position::Bottom)
			label.label->set(textForValue(label.value), textColor);
	}
	if (label.line) {
		label.lineStart = label.line->line();
		label.lineEnd = linePos(pos);
	}
}

DiveCartesianAxis::Label DiveCartesianAxis::createLabel(double value, double pos, double dataMinOld, double dataMaxOld, int animSpeed, bool noLabel)
{
	Label label { value, 0.0, 1.0 };
	double posStart = posAtValue(value, dataMaxOld, dataMinOld);
	if (textVisibility && !noLabel) {
		label.labelPosStart = labelPos(posStart);
		label.labelPosEnd = labelPos(pos);
		int alignFlags = position == Position::Bottom ? Qt::AlignTop | Qt::AlignHCenter :
				 position == Position::Left   ? Qt::AlignVCenter | Qt::AlignLeft:
								Qt::AlignVCenter | Qt::AlignRight;
		label.label = std::make_unique<DiveTextItem>(dpr, labelScale, alignFlags, this);
		label.label->set(textForValue(value), textColor);
		label.label->setZValue(1);
		label.label->setPos(animSpeed <= 0 ? label.labelPosEnd : label.labelPosStart);
		label.label->setOpacity(animSpeed <= 0 ? 1.0 : 0.0);
	}
	if (lineVisibility) {
		label.lineStart = linePos(posStart);
		label.lineEnd = linePos(pos);
		label.line = std::make_unique<DiveLineItem>(this);
		label.line->setPen(gridPen);
		label.line->setZValue(0);
		label.line->setLine(animSpeed <= 0 ? label.lineEnd : label.lineStart);
		label.line->setOpacity(animSpeed <= 0 ? 1.0 : 0.0);
	}
	return label;
}

void DiveCartesianAxis::updateLabels(int numTicks, double firstPosScreen, double firstValue, double stepScreen, double stepValue,
				     int animSpeed, double dataMinOld, double dataMaxOld)
{
	if (animSpeed <= 0)
		labels.clear(); // No animation? Simply redo the labels.

	std::vector<Label> newLabels;
	newLabels.reserve(numTicks);
	auto actOld = labels.begin();
	double value = firstValue;

	for (int i = 0; i < numTicks; i++, value += stepValue) {

		// Check if we already got that label. If we find unused labels, mark them for deletion.
		// Labels to be deleted are recognized by an end-opacity of 0.0.
		// Note: floating point comparisons should be fine owing to our rounding to integers above.
		for ( ; actOld != labels.end() && actOld->value < value; ++actOld) {
			double pos = posAtValue(actOld->value);
			updateLabel(*actOld, 0.0, pos);
			newLabels.push_back(std::move(*actOld));
		}

		double pos = ((position == Position::Bottom) != inverted) ?
					 firstPosScreen + i * stepScreen :
					 firstPosScreen - i * stepScreen;
		if (actOld != labels.end() && actOld->value == value) {
			// Update label, but don't delete it
			updateLabel(*actOld, 1.0, pos);
			newLabels.push_back(std::move(*actOld));
			++actOld;
		} else {
			// This is horrible: for the depth axis, we don't want to show the first label (0).
			// We recognize this by the fact that the depth axis is the only "inverted" axis.
			// This really should be replaced by a general flag to avoid surprises!
			bool noLabel = inverted && i == 0;

			// Create new label
			newLabels.push_back(createLabel(value, pos, dataMinOld, dataMaxOld, animSpeed, noLabel));
		}
	}

	// If there are any labels left, mark them for deletion.
	for ( ; actOld != labels.end(); ++actOld) {
		double pos = posAtValue(actOld->value);
		updateLabel(*actOld, 0.0, pos);
		newLabels.push_back(std::move(*actOld));
	}

	labels = std::move(newLabels);
}

// Arithmetics with lines. Needed for animations. Operates pointwise.
static QLineF operator-(const QLineF &l1, const QLineF &l2)
{
	return QLineF(l1.p1() - l2.p1(), l1.p2() - l2.p2());
}
static QLineF operator+(const QLineF &l1, const QLineF &l2)
{
	return QLineF(l1.p1() + l2.p1(), l1.p2() + l2.p2());
}
static QLineF operator*(double f, const QLineF &l)
{
	return QLineF(f*l.p1(), f*l.p2());
}

// Helper template: get point in interval (0.0: start, 1.0: end)
template <typename T>
T mid(const T &start, const T &end, double fraction)
{
	return start + fraction * (end - start);
}

void DiveCartesianAxis::anim(double fraction)
{
	if (fraction == 1.0) {
		// The animation has finished.
		// Remove labels that have been marked for deletion by setting the opacity to 0.0.
		// Use the erase-remove idiom (yes, it is a weird idiom).
		labels.erase(std::remove_if(labels.begin(), labels.end(),
					    [](const Label &l) { return l.opacityEnd == 0.0; }),
			     labels.end());
	}
	for (Label &label: labels) {
		double opacity = mid(label.opacityStart, label.opacityEnd, fraction);
		if (label.label) {
			label.label->setOpacity(opacity);
			label.label->setPos(mid(label.labelPosStart, label.labelPosEnd, fraction));
		}
		if (label.line) {
			label.line->setOpacity(opacity);
			label.line->setLine(mid(label.lineStart, label.lineEnd, fraction));
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

double DiveCartesianAxis::valueAt(const QPointF &p) const
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

double DiveCartesianAxis::deltaToValue(double delta) const
{
	QLineF m = line();
	double screenSize = position == Position::Bottom ? m.x2() - m.x1()
							 : m.y2() - m.y1();
	double axisSize = max - min;
	double res = delta * axisSize / screenSize;
	return ((position == Position::Bottom) == inverted) ? -res : res;
}

double DiveCartesianAxis::posAtValue(double value, double max, double min) const
{
	QLineF m = line();

	double screenFrom = position == Position::Bottom ? m.x1() : m.y1();
	double screenTo = position == Position::Bottom ? m.x2() : m.y2();
	if (nearly_equal(min, max))
		return (screenFrom + screenTo) / 2.0;
	if ((position == Position::Bottom) == inverted)
		std::swap(screenFrom, screenTo);
	return (value - min) / (max - min) *
	       (screenTo - screenFrom) + screenFrom;
}

double DiveCartesianAxis::posAtValue(double value) const
{
	return posAtValue(value, max, min);
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

void DiveCartesianAxis::setGridIsMultipleOfThree(bool arg1)
{
	gridIsMultipleOfThree = arg1;
}

std::pair<double, double> DiveCartesianAxis::screenMinMax() const
{
	return position == Position::Bottom ? std::make_pair(rect.left(), rect.right())
					    : std::make_pair(rect.top(), rect.bottom());
}
