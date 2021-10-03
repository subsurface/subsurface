// SPDX-License-Identifier: GPL-2.0
#ifndef DIVECARTESIANAXIS_H
#define DIVECARTESIANAXIS_H

#include <QObject>
#include <QGraphicsLineItem>
#include <QPen>
#include "core/color.h"
#include "core/units.h"

class ProfileScene;
class QPropertyAnimation;
class DiveTextItem;
class DiveLineItem;
class DivePlotDataModel;

class DiveCartesianAxis : public QObject, public QGraphicsLineItem {
	Q_OBJECT
	Q_PROPERTY(QLineF line WRITE setLine READ line)
	Q_PROPERTY(QPointF pos WRITE setPos READ pos)
	Q_PROPERTY(qreal x WRITE setX READ x)
	Q_PROPERTY(qreal y WRITE setY READ y)
private:
	bool printMode;
public:
	enum Orientation {
		TopToBottom,
		BottomToTop,
		LeftToRight,
		RightToLeft
	};
	enum class Position {
		Left, Right, Bottom
	};

	DiveCartesianAxis(Position position, int integralDigits, int fractionalDigits, color_index_t gridColor,
			  QColor textColor, bool textVisible, bool linesVisible,
			  double dpr, double labelScale, bool printMode, bool isGrayscale, ProfileScene &scene);
	~DiveCartesianAxis();
	void setBounds(double min, double max);
	void setTransform(double a, double b = 0.0);
	void setOrientation(Orientation orientation);
	double minimum() const;
	double maximum() const;
	std::pair<double, double> screenMinMax() const;
	qreal valueAt(const QPointF &p) const;
	qreal posAtValue(qreal value) const;
	void setPosition(const QRectF &rect);
	void setTextVisible(bool arg1);
	void setLinesVisible(bool arg1);
	void updateTicks(int animSpeed);
	double width() const; // only for vertical axes
	double height() const; // only for horizontal axes

private:
	Position position;
	int fractionalDigits;
	QRectF rect; // Rectangle to fill with grid lines
	QPen gridPen;
	QColor textColor;
	ProfileScene &scene;
	QString textForValue(double value) const;
	Orientation orientation;
	QList<DiveTextItem *> labels;
	QList<DiveLineItem *> lines;
	double dataMin, dataMax;
	double min, max;
	bool textVisibility;
	bool lineVisibility;
	double labelScale;
	bool changed;
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

	void updateLabels(int numTicks, double firstPosScreen, double firstValue, double stepScreen, double stepValue, int animSpeed);
	void updateLines(int numTicks, double firstPosScreen, double stepScreen, int animSpeed);
};

#endif // DIVECARTESIANAXIS_H
