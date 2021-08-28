// SPDX-License-Identifier: GPL-2.0
#ifndef DIVECARTESIANAXIS_H
#define DIVECARTESIANAXIS_H

#include <QObject>
#include <QGraphicsLineItem>
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
	QPen gridPen() const;
public:
	enum Orientation {
		TopToBottom,
		BottomToTop,
		LeftToRight,
		RightToLeft
	};
	DiveCartesianAxis(double dpr, bool printMode, ProfileScene &scene);
	~DiveCartesianAxis();
	void setMinimum(double minimum);
	void setMaximum(double maximum);
	void setTickInterval(double interval);
	void setOrientation(Orientation orientation);
	void setTickSize(qreal size);
	void setFontLabelScale(qreal scale);
	double minimum() const;
	double maximum() const;
	qreal valueAt(const QPointF &p) const;
	qreal posAtValue(qreal value) const;
	void setColor(const QColor &color);
	void setTextColor(const QColor &color);
	void animateChangeLine(const QLineF &newLine, int animSpeed);
	void setTextVisible(bool arg1);
	void setLinesVisible(bool arg1);
	void setLineSize(qreal lineSize);
	void setLine(const QLineF& line);
	virtual void updateTicks(int animSpeed, color_index_t color = TIME_GRID);
	double width() const; // only for vertical axes
	double height() const; // only for horizontal axes

signals:
	void sizeChanged();

protected:
	ProfileScene &scene;
	virtual QString textForValue(double value) const;
	virtual QColor colorForValue(double value) const;
	double textWidth(const QString &s) const;
	Orientation orientation;
	QList<DiveTextItem *> labels;
	QList<DiveLineItem *> lines;
	double min;
	double max;
	double interval;
	double tick_size;
	QColor textColor;
	bool textVisibility;
	bool lineVisibility;
	double labelScale;
	qreal line_size;
	bool changed;
	double dpr;
};

class DepthAxis : public DiveCartesianAxis {
	Q_OBJECT
public:
	DepthAxis(double dpr, bool printMode, ProfileScene &scene);
private:
	QString textForValue(double value) const override;
	QColor colorForValue(double value) const override;
};

class TimeAxis : public DiveCartesianAxis {
	Q_OBJECT
public:
	using DiveCartesianAxis::DiveCartesianAxis;
	void updateTicks(int animSpeed, color_index_t color = TIME_GRID) override;
private:
	QString textForValue(double value) const override;
	QColor colorForValue(double value) const override;
};

class TemperatureAxis : public DiveCartesianAxis {
	Q_OBJECT
public:
	using DiveCartesianAxis::DiveCartesianAxis;
private:
	QString textForValue(double value) const override;
};

class PartialGasPressureAxis : public DiveCartesianAxis {
	Q_OBJECT
public:
	PartialGasPressureAxis(const DivePlotDataModel &model, double dpr, bool printMode, ProfileScene &scene);
	void update(int animSpeed);
	double width() const;
private:
	const DivePlotDataModel &model;
};

#endif // DIVECARTESIANAXIS_H
