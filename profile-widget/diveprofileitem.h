// SPDX-License-Identifier: GPL-2.0
#ifndef DIVEPROFILEITEM_H
#define DIVEPROFILEITEM_H

#include <QGraphicsPolygonItem>
#include <memory>

#include "divelineitem.h"

#include "core/equipment.h"

/* This is the Profile Item, it should be used for quite a lot of things
 on the profile view. The usage should be pretty simple:

 DiveProfileItem *profile = new DiveProfileItem(DiveDataModel, timeAxis, DiveDataModel::TIME, DiveDataModel, DiveDataModel::DEPTH, dpr);
 scene()->addItem(profile);

 This is a generically item and should be used as a base for others, I think...
*/

class DiveTextItem;
class DiveCartesianAxis;
struct plot_data;
struct plot_info;
struct dive;

class AbstractProfilePolygonItem : public QGraphicsPolygonItem {
public:
	using DataAccessor = double (*)(const plot_data &data); // The pointer-to-function syntax is hilarious.
	AbstractProfilePolygonItem(const plot_info &pInfo, const DiveCartesianAxis &hAxis, const DiveCartesianAxis &vAxis,
				   DataAccessor accessor, double dpr);
	~AbstractProfilePolygonItem();
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
	const plot_info &pInfo;
	DataAccessor accessor;
	double dpr;
	int from, to;
	std::vector<std::unique_ptr<DiveTextItem>> texts;
};

class DiveProfileItem : public AbstractProfilePolygonItem {
public:
	DiveProfileItem(const plot_info &pInfo, const DiveCartesianAxis &hAxis, const DiveCartesianAxis &vAxis,
			DataAccessor accessor, double dpr);
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void plot_depth_sample(const struct plot_data &entry, QFlags<Qt::AlignmentFlag> flags, const QColor &color);
	int maxCeiling(int row);

private:
	QColor profileColor;
};

class DiveMeanDepthItem : public AbstractProfilePolygonItem {
public:
	DiveMeanDepthItem(const plot_info &pInfo, const DiveCartesianAxis &hAxis, const DiveCartesianAxis &vAxis,
			  DataAccessor accessor, double dpr);
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;
	double labelWidth;

private:
	void createTextItem(double lastSec, double lastMeanDepth);
	std::pair<double,double> getMeanDepth(int i) const;
	std::pair<double,double> getNextMeanDepth(int i) const;
};

class DiveTemperatureItem : public AbstractProfilePolygonItem {
public:
	DiveTemperatureItem(const plot_info &pInfo, const DiveCartesianAxis &hAxis, const DiveCartesianAxis &vAxis,
			    DataAccessor accessor, double dpr);
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;

private:
	void createTextItem(int seconds, int mkelvin, bool last);
};

class DiveHeartrateItem : public AbstractProfilePolygonItem {
public:
	DiveHeartrateItem(const plot_info &pInfo, const DiveCartesianAxis &hAxis, const DiveCartesianAxis &vAxis,
			  DataAccessor accessor, double dpr);
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
	void createTextItem(int seconds, int hr, bool last);
};

class DiveGasPressureItem : public AbstractProfilePolygonItem {
public:
	using AbstractProfilePolygonItem::AbstractProfilePolygonItem;
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;

private:
	double plotPressureValue(double mbar, double sec, QFlags<Qt::AlignmentFlag> align, double y_offset);
	void plotGasValue(double mbar, double sec, const cylinder_t *cylinder, QFlags<Qt::AlignmentFlag> align, double x_offset, double y_offset, bool showDescription);
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
public:
	DiveCalculatedCeiling(const plot_info &pInfo, const DiveCartesianAxis &hAxis,
			      const DiveCartesianAxis &vAxis, DataAccessor accessor, double dpr);
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;
};

class DiveReportedCeiling : public AbstractProfilePolygonItem {
public:
	DiveReportedCeiling(const plot_info &pInfo, const DiveCartesianAxis &hAxis, const DiveCartesianAxis &vAxis, DataAccessor accessor, double dpr);
	void replot(const dive *d, int from, int to, bool in_planner) override;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0) override;
private:
	std::pair<double,double> getTimeValue(int i) const;
	std::pair<double, double> getPoint(int i) const;
};

class DiveCalculatedTissue : public DiveCalculatedCeiling {
public:
	DiveCalculatedTissue(const plot_info &pInfo, const DiveCartesianAxis &hAxis,
			     const DiveCartesianAxis &vAxis, DataAccessor accessor, double dpr);
};

class PartialPressureGasItem : public AbstractProfilePolygonItem {
public:
	PartialPressureGasItem(const plot_info &pInfo, const DiveCartesianAxis &hAxis, const DiveCartesianAxis &vAxis, DataAccessor accessor, double dpr);
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
