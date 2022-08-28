// SPDX-License-Identifier: GPL-2.0
#ifndef DIVECARTESIANAXIS_H
#define DIVECARTESIANAXIS_H

#include <memory>
#include <QGraphicsLineItem>
#include <QPen>
#include "core/color.h"
#include "core/units.h"

class ProfileScene;
class DiveTextItem;
class DiveLineItem;

class DiveCartesianAxis : public QGraphicsLineItem {
private:
	bool printMode;
public:
	enum class Position {
		Left, Right, Bottom
	};

	DiveCartesianAxis(Position position, bool inverted, int integralDigits, int fractionalDigits, color_index_t gridColor,
			  QColor textColor, bool textVisible, bool linesVisible,
			  double dpr, double labelScale, bool printMode, bool isGrayscale, ProfileScene &scene);
	~DiveCartesianAxis();
	void setBounds(double min, double max);
	void setTransform(double a, double b = 0.0);
	double minimum() const;
	double maximum() const;
	std::pair<double, double> screenMinMax() const;
	double valueAt(const QPointF &p) const;
	double posAtValue(double value) const;
	double deltaToValue(double delta) const; // For panning: turn a screen distance to delta-value
	void setPosition(const QRectF &rect);
	double screenPosition(double pos) const; // 0.0 = begin, 1.0 = end of axis, independent of represented values
	double pointInRange(double pos) const; // Point on screen is in range of axis
	void setTextVisible(bool arg1);
	void setLinesVisible(bool arg1);
	void setGridIsMultipleOfThree(bool arg1);
	void updateTicks(int animSpeed);
	double width() const; // only for vertical axes
	double height() const; // only for horizontal axes
	double horizontalOverhang() const; // space needed for labels of horizontal axes
	void anim(double fraction);

	// The minimum space between two labels on the plot in seconds
	int getMinLabelDistance(const DiveCartesianAxis &timeAxis) const;

private:
	struct Label {
		double value;
		double opacityStart;
		double opacityEnd;	// If 0.0, label will be removed at end of animation
		QPointF labelPosStart;
		QPointF labelPosEnd;
		QLineF lineStart;
		QLineF lineEnd;
		std::unique_ptr<DiveTextItem> label;
		std::unique_ptr<DiveLineItem> line;
	};
	Position position;
	bool inverted; // Top-to-bottom or right-to-left axis.
	int fractionalDigits;
	QRectF rect; // Rectangle to fill with grid lines
	QPen gridPen;
	QColor textColor;
	ProfileScene &scene;
	double posAtValue(double value, double max, double min) const;
	QPointF labelPos(double pos) const;
	QLineF linePos(double pos) const;
	void updateLabel(Label &label, double opacityEnd, double pos) const;
	Label createLabel(double value, double pos, double dataMinOld, double dataMaxOld, int animSpeed, bool noLabel);
	QString textForValue(double value) const;
	std::vector<Label> labels;
	double dataMin, dataMax;
	double min, max;
	bool textVisibility;
	bool lineVisibility;
	bool gridIsMultipleOfThree;
	double labelScale;
	double dpr;
	double labelWidth, labelHeight; // maximum expected sizes of label width and height

	// To format the labels and choose the label positions, the
	// axis has to be aware of the displayed values. Thankfully,
	// the conversion between internal data (eg. mm) and displayed
	// data (e.g. ft) can be represented by an affine map ax+b.
	struct Transform {
		double a, b;
		double to(double x) const;
		double from(double y) const;
	} transform;

	void updateLabels(int numTicks, double firstPosScreen, double firstValue, double stepScreen, double stepValue,
			  int animSpeed, double dataMinOld, double dataMaxOld);
};

#endif // DIVECARTESIANAXIS_H
