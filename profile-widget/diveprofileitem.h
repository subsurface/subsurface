// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEPROFILEITEM_H
#define DIVEPROFILEITEM_H

#include <QObject>
#include <QGraphicsPolygonItem>
#include <QModelIndex>

#include "divelineitem.h"

/* This is the Profile Item, it should be used for quite a lot of things
 on the profile view. The usage should be pretty simple:

 DiveProfileItem *profile = new DiveProfileItem(DiveDataModel, timeAxis, DiveDataModel::TIME, DiveDataModel, DiveDataModel::DEPTH, dpr);
 scene()->addItem(profile);

 This is a generically item and should be used as a base for others, I think...
*/

class DivePlotDataModel;
class DiveTextItem;
class DiveCartesianAxis;
class QAbstractTableModel;
struct plot_data;
struct dive;

class AbstractProfilePolygonItem : public QObject, public QGraphicsPolygonItem {
	Q_OBJECT
	Q_PROPERTY(QPointF pos WRITE setPos READ pos)
	Q_PROPERTY(qreal x WRITE setX READ x)
	Q_PROPERTY(qreal y WRITE setY READ y)
public:
	AbstractProfilePolygonItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn, double dpr);
	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) = 0;
	void clear();

	// Plot the range (from, to), given as indexes. The caller guarantees that
	// only the first and the last segment will have to be clipped.
	virtual void replot(const dive *d, int from, int to, bool in_planner) = 0;

protected:
	void makePolygon(int from, int to);
	void clipStart(double &x, double &y, double next_x, double next_y) const;
	void clipStop(double &x, double &y, double prev_x, double prev_y) const;
	std::pair<double, double> getPoint(int i) const;
	const DiveCartesianAxis &hAxis;
	const DiveCartesianAxis &vAxis;
	const DivePlotDataModel &dataModel;
	int hDataColumn;
	int vDataColumn;
	double dpr;
	int from, to;
	QList<DiveTextItem *> texts;
};

class DiveProfileItem : public AbstractProfilePolygonItem {
	Q_OBJECT

public:
	DiveProfileItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn, double dpr);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void plot_depth_sample(const struct plot_data &entry, QFlags<Qt::AlignmentFlag> flags, const QColor &color);
	int maxCeiling(int row);

private:
	unsigned int show_reported_ceiling;
	unsigned int reported_ceiling_in_red;
	QColor profileColor;
};

class DiveMeanDepthItem : public AbstractProfilePolygonItem {
	Q_OBJECT
public:
	DiveMeanDepthItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn, double dpr);
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;
	double labelWidth;

private:
	void createTextItem(double lastSec, double lastMeanDepth);
	std::pair<double,double> getMeanDepth(int i) const;
	std::pair<double,double> getNextMeanDepth(int i) const;
};

class DiveTemperatureItem : public AbstractProfilePolygonItem {
	Q_OBJECT
public:
	DiveTemperatureItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn, double dpr);
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;

private:
	void createTextItem(int seconds, int mkelvin, bool last);
};

class DiveHeartrateItem : public AbstractProfilePolygonItem {
	Q_OBJECT
public:
	DiveHeartrateItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn, double dpr);
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
	void createTextItem(int seconds, int hr, bool last);
};

class DiveGasPressureItem : public AbstractProfilePolygonItem {
	Q_OBJECT

public:
	using AbstractProfilePolygonItem::AbstractProfilePolygonItem;
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;

private:
	void plotPressureValue(double mbar, double sec, QFlags<Qt::AlignmentFlag> align, double offset);
	void plotGasValue(double mbar, double sec, struct gasmix gasmix, QFlags<Qt::AlignmentFlag> align, double offset);
	struct PressureEntry {
		double time = 0.0;
		double pressure = 0.0;
	};
	struct Entry {
		QPointF pos;
		QColor col;
	};
	struct Segment {
		int cyl;
		std::vector<Entry> polygon;
		PressureEntry first, last;
	};
	std::vector<Segment> segments;
};

class DiveCalculatedCeiling : public AbstractProfilePolygonItem {
	Q_OBJECT

public:
	DiveCalculatedCeiling(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn,
			      const DiveCartesianAxis &vAxis, int vColumn, double dpr);
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;
};

class DiveReportedCeiling : public AbstractProfilePolygonItem {
	Q_OBJECT

public:
	DiveReportedCeiling(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn, double dpr);
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;
private:
	std::pair<double,double> getTimeValue(int i) const;
	std::pair<double, double> getPoint(int i) const;
};

class DiveCalculatedTissue : public DiveCalculatedCeiling {
	Q_OBJECT
public:
	DiveCalculatedTissue(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn,
			     const DiveCartesianAxis &vAxis, int vColumn, double dpr);
};

class PartialPressureGasItem : public AbstractProfilePolygonItem {
	Q_OBJECT
public:
	PartialPressureGasItem(const DivePlotDataModel &model, const DiveCartesianAxis &hAxis, int hColumn, const DiveCartesianAxis &vAxis, int vColumn, double dpr);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void setThresholdSettingsKey(const double *prefPointerMin, const double *prefPointerMax);
	void setVisibilitySettingsKey(const QString &setVisibilitySettingsKey);
	void setColors(const QColor &normalColor, const QColor &alertColor);

private:
	QVector<QPolygonF> alertPolygons;
	const double *thresholdPtrMin;
	const double *thresholdPtrMax;
	QColor normalColor;
	QColor alertColor;
};
#endif // DIVEPROFILEITEM_H
