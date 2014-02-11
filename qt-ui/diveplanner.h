#ifndef DIVEPLANNER_H
#define DIVEPLANNER_H

#include <QGraphicsView>
#include <QGraphicsPathItem>
#include <QDialog>
#include <QAbstractTableModel>
#include <QDateTime>

#include "dive.h"

class QListView;
class QModelIndex;

class DivePlannerPointsModel : public QAbstractTableModel{
	Q_OBJECT
public:
	static DivePlannerPointsModel* instance();
	enum Sections{REMOVE, DEPTH, DURATION, GAS, CCSETPOINT, COLUMNS};
	enum Mode { NOTHING, PLAN, ADD };
	virtual int columnCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
	virtual Qt::ItemFlags flags(const QModelIndex& index) const;
	void removeSelectedPoints(const QVector<int>& rows);
	void setPlanMode(Mode mode);
	bool isPlanner();
	void createSimpleDive();
	void clear();
	Mode currentMode() const;
	void tanksUpdated();
	void rememberTanks();
	bool tankInUse(int o2, int he);
	void copyCylinders(struct dive *d);
	/**
	 * @return the row number.
	 */
	void editStop(int row, divedatapoint newData );
	divedatapoint at(int row);
	int size();
	struct diveplan getDiveplan();
	QStringList &getGasList();
	QVector<QPair<int, int> > collectGases(dive *d);

public slots:
	int addStop(int millimeters = 0, int seconds = 0, int o2 = 0, int he = 0, int ccpoint = 0 );
	void addCylinder_clicked();
	void setGFHigh(const int gfhigh);
	void setGFLow(const int ghflow);
	void setSurfacePressure(int pressure);
	void setBottomSac(int sac);
	void setDecoSac(int sac);
	void setStartTime(const QTime& t);
	void setLastStop6m(bool value);
	void createPlan();
	void remove(const QModelIndex& index);
	void cancelPlan();
	void createTemporaryPlan();
	void deleteTemporaryPlan();
	void loadFromDive(dive* d);
	void restoreBackupDive();
signals:
	void planCreated();
	void planCanceled();
private:
	explicit DivePlannerPointsModel(QObject* parent = 0);
	bool addGas(int o2, int he);
	struct diveplan diveplan;
	Mode mode;
	QVector<divedatapoint> divepoints;
	struct dive *tempDive;
	struct dive backupDive;
	void deleteTemporaryPlan(struct divedatapoint *dp);
	QVector<sample> backupSamples; // For editing added dives.
	struct dive *stagingDive;
	QVector<QPair<int, int> > oldGases;
};

class Button : public QObject, public QGraphicsRectItem {
	Q_OBJECT
public:
	Button(QObject* parent = 0, QGraphicsItem *itemParent = 0);
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


class ExpanderGraphics : public QGraphicsRectItem {
public:
	ExpanderGraphics(QGraphicsItem *parent = 0);

	QGraphicsPixmapItem *icon;
	Button *increaseBtn;
	Button *decreaseBtn;
private:
	QGraphicsPixmapItem *bg;
	QGraphicsPixmapItem *leftWing;
	QGraphicsPixmapItem *rightWing;
};

class DiveHandler : public QObject, public QGraphicsEllipseItem{
Q_OBJECT
public:
	DiveHandler();
protected:
	void mousePressEvent(QGraphicsSceneMouseEvent* event);
	void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);
private:
	int parentIndex();
public slots:
	void selfRemove();
	void changeGas();
};

class Ruler : public QGraphicsLineItem{
public:
	Ruler();
	~Ruler();
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
	void setTextColor(const QColor& color);
	int unitSystem;

private:
	void eraseAll();

	Qt::Orientation orientation;
	QList<QGraphicsLineItem*> ticks;
	QList<QGraphicsSimpleTextItem*> labels;
	double min;
	double max;
	double interval;
	double tickSize;
	QColor textColor;
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
	qreal fromPercent(qreal percent, Qt::Orientation orientation);
public slots:
	void settingsChanged();
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
	void decreaseDepth();
	void drawProfile();
	void pointInserted(const QModelIndex&, int start, int end);
	void pointsRemoved(const QModelIndex&, int start, int end);
private:
	void moveActiveHandler(const QPointF& MappedPos, const int pos);

	/* This are the lines of the plotted dive. */
	QList<QGraphicsLineItem*> lines;

	/* This is the user-entered handles. */
	QList<DiveHandler *> handles;
	QList<QGraphicsSimpleTextItem*> gases;

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
	ExpanderGraphics *depthHandler;
	ExpanderGraphics *timeHandler;

	int minMinutes; // this holds the minimum requested window time
	int minDepth; // this holds the minimum requested window depth
	int dpMaxTime; // this is the time of the dive calculated by the deco.

	friend class DiveHandler;
};

#include "ui_diveplanner.h"

class DivePlannerWidget : public QWidget {
	Q_OBJECT
public:
    explicit DivePlannerWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);

public slots:
	void settingsChanged();
	void atmPressureChanged(const QString& pressure);
	void bottomSacChanged(const QString& bottomSac);
	void decoSacChanged(const QString& decosac);
private:
	Ui::DivePlanner ui;
};

#endif // DIVEPLANNER_H
