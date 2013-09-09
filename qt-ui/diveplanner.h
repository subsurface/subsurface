#ifndef DIVEPLANNER_H
#define DIVEPLANNER_H

#include <QGraphicsView>
#include <QGraphicsPathItem>
#include <QDialog>
#include <QAbstractTableModel>
#include <QDateTime>

#include "dive.h"

namespace Ui{
	class DivePlanner;
};

class QListView;
class QStringListModel;
class QModelIndex;

// Return a Model containing the air types.
QStringListModel *airTypes();

class DivePlannerPointsModel : public QAbstractTableModel{
	Q_OBJECT
public:
	static DivePlannerPointsModel* instance();
	enum Sections{REMOVE, DEPTH, DURATION, GAS, CCSETPOINT, COLUMNS};
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;
	void removeSelectedPoints(const QVector<int>& rows);

	/**
	 * @return the row number.
	 */
	int addStop(int meters, int minutes,const QString& gas, int ccpoint );
	void editStop(int row, divedatapoint newData );
	divedatapoint at(int row);
	struct diveplan getDiveplan();
public slots:
	void setGFHigh(short gfhigh);
	void setGFLow(short ghflow);
	void setSurfacePressure(int pressure);
	void setBottomSac(int sac);
	void setDecoSac(int sac);
	void setStartTime(const QTime& t);
	void setLastStop6m(bool value);
	void createPlan();
	void remove(const QModelIndex& index);

private:
	explicit DivePlannerPointsModel(QObject* parent = 0);
	struct diveplan diveplan;
	QVector<divedatapoint> divepoints;
};

class Button : public QObject, public QGraphicsRectItem {
	Q_OBJECT
public:
	explicit Button(QObject* parent = 0);
	void setText(const QString& text);
	void setPixmap(const QPixmap& pixmap);

protected:
	virtual void mousePressEvent(QGraphicsSceneMouseEvent* event);
signals:
	void clicked();
private:
	QGraphicsPixmapItem *icon;
	QGraphicsSimpleTextItem *text;
};

class DiveHandler : public QGraphicsEllipseItem{
public:
	DiveHandler();
protected:
	void mousePressEvent(QGraphicsSceneMouseEvent* event);
};

class Ruler : public QGraphicsLineItem{
public:
	Ruler();
	void setMinimum(double minimum);
	void setMaximum(double maximum);
	void setTickInterval(double interval);
	void setOrientation(Qt::Orientation orientation);
	void setTickSize(qreal size);
	void updateTicks();
	double minimum() const;
	double maximum() const;
	qreal valueAt(const QPointF& p);
	qreal percentAt(const QPointF& p);
	qreal posAtValue(qreal value);
	void setColor(const QColor& color);

private:
	Qt::Orientation orientation;
	QList<QGraphicsLineItem*> ticks;
	double min;
	double max;
	double interval;
	double posBegin;
	double posEnd;
	double tickSize;
};

class DivePlannerGraphics : public QGraphicsView {
	Q_OBJECT
public:
	DivePlannerGraphics(QWidget* parent = 0);
protected:
	virtual void mouseDoubleClickEvent(QMouseEvent* event);
	virtual void showEvent(QShowEvent* event);
	virtual void resizeEvent(QResizeEvent* event);
	virtual void mouseMoveEvent(QMouseEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void mouseReleaseEvent(QMouseEvent* event);
	bool isPointOutOfBoundaries(const QPointF& point);
	void deleteTemporaryDivePlan(struct divedatapoint* dp);
	qreal fromPercent(qreal percent, Qt::Orientation orientation);
private slots:
	void keyEscAction();
	void keyDeleteAction();
	void keyUpAction();
	void keyDownAction();
	void keyLeftAction();
	void keyRightAction();
	void increaseTime();
	void increaseDepth();
	void decreaseTime();
	void decreaseDepth();;
	void createDecoStops();
	void cancelPlan();
	void prepareSelectGas();
	void selectGas(const QModelIndex& index);
	void pointInserted(const QModelIndex&, int start, int end);
	void pointsRemoved(const QModelIndex&, int start, int end);
private:
	void moveActiveHandler(const QPointF& pos);

	/* This are the lines of the plotted dive. */
	QList<QGraphicsLineItem*> lines;

	/* This is the user-entered handles. */
	QList<DiveHandler *> handles;

	/* this is the user-entered gases.
		This must be a button, so the
		user cna click to choose a new gas.
	 */
	QList<Button*> gases;
	QListView *gasListView;
	QStringListModel *gasChoices;
	Button *currentGasChoice;

	/* those are the lines that follows the mouse. */
	QGraphicsLineItem *verticalLine;
	QGraphicsLineItem *horizontalLine;

	/* This is the handler that's being dragged. */
	DiveHandler *activeDraggedHandler;

	// When we start to move the handler, this pos is saved.
	// so we can revert it later.
	QPointF originalHandlerPos;

	/* this is the background of the dive, the blue-gradient. */
	QGraphicsPolygonItem *diveBg;

	/* This is the bottom ruler - the x axis, and it's associated text */
	Ruler *timeLine;
	QGraphicsSimpleTextItem *timeString;

	/* this is the left ruler, the y axis, and it's associated text. */
	Ruler *depthLine;
	QGraphicsSimpleTextItem *depthString;

	/* Buttons */
	Button *plusTime;  // adds 10 minutes to the time ruler.
	Button *plusDepth; // adds 10 meters to the depth ruler.
	Button *lessTime;  // remove 10 minutes to the time ruler.
	Button *lessDepth; // remove 10 meters to the depth ruler.

	int minMinutes; // this holds the minimum duration of the dive.
	int dpMaxTime; // this is the time of the dive calculated by the deco.
};

class DivePlannerWidget : public QWidget {
	Q_OBJECT
public:
    explicit DivePlannerWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);

public slots:
	void startTimeChanged(const QTime& time);
	void atmPressureChanged(const QString& pressure);
	void bottomSacChanged(const QString& bottomSac);
	void decoSacChanged(const QString& decosac);
	void gflowChanged(const QString& gflow);
	void gfhighChanged(const QString& gfhigh);
	void lastStopChanged(bool checked);
private:
	Ui::DivePlanner *ui;
};

#endif
