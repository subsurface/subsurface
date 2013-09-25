#ifndef PROFILEGRAPHICS_H
#define PROFILEGRAPHICS_H

#include "graphicsview-common.h"
#include "../display.h"
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QIcon>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QGraphicsProxyWidget>

struct text_render_options;
struct graphics_context;
struct plot_info;
typedef struct text_render_options text_render_options_t;

/* To use a tooltip, simply ->setToolTip on the QGraphicsItem that you want
 * or, if it's a "global" tooltip, set it on the mouseMoveEvent of the ProfileGraphicsView.
 */
class ToolTipItem :public QObject, public QGraphicsPathItem
{
	Q_OBJECT
	void updateTitlePosition();
	Q_PROPERTY(QRectF rect READ boundingRect WRITE setRect)

public:
	enum Status{COLLAPSED, EXPANDED};
	enum {ICON_SMALL = 16, ICON_MEDIUM = 24, ICON_BIG = 32, SPACING=4};

	explicit ToolTipItem(QGraphicsItem* parent = 0);
	virtual ~ToolTipItem();

	void collapse();
	void expand();
	void clear();
	void addToolTip(const QString& toolTip, const QIcon& icon = QIcon());
	void refresh(struct graphics_context* gc, QPointF pos);
	bool isExpanded();
	void persistPos();
	void readPos();
	void mousePressEvent(QGraphicsSceneMouseEvent* event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	bool eventFilter(QObject* , QEvent* );
public slots:
	void setRect(const QRectF& rect);

private:
	typedef QPair<QGraphicsPixmapItem*, QGraphicsSimpleTextItem*> ToolTip;
	QList<ToolTip> toolTips;
	QGraphicsPathItem *background;
	QGraphicsLineItem *separator;
	QGraphicsSimpleTextItem *title;
	Status status;
	QRectF rectangle;
	bool dragging;
	QRectF nextRectangle;
};

class RulerItem;

class RulerNodeItem : public QObject, public QGraphicsEllipseItem
{
	Q_OBJECT
	friend class RulerItem;
public:
	explicit RulerNodeItem(QGraphicsItem* parent, graphics_context gc);
	void setRuler(RulerItem *r);
	void recalculate();

protected:
	QVariant itemChange(GraphicsItemChange change, const QVariant & value );

private:
	graphics_context gc;
	struct plot_data *entry;
	RulerItem* ruler;
};

class RulerItem : public QGraphicsObject
{
	Q_OBJECT
public:
	explicit RulerItem(QGraphicsItem* parent,
			   RulerNodeItem *sourceMarker,
			   RulerNodeItem *destMarker);
	void recalculate();

	RulerNodeItem* sourceNode() const;
	RulerNodeItem* destNode() const;
	void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget * widget = 0);
	QRectF boundingRect() const;
	QPainterPath shape() const;

private:
	QPointF startPoint, endPoint;
	RulerNodeItem *source, *dest;
	QString text;
	int height;
	int paint_direction;
};

class EventItem : public QGraphicsPolygonItem
{
public:
	explicit EventItem(QGraphicsItem* parent = 0, bool grayscale = FALSE);

private:
	ToolTipItem *controller;
	QString text;
	QIcon icon;
	bool isGrayscale;

	QColor getColor(const color_indice_t i);
};

class GraphicsTextEditor : public QGraphicsTextItem{
	Q_OBJECT
public:
    GraphicsTextEditor(QGraphicsItem* parent = 0);

protected:
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

signals:
	void textChanged(const QString& text);
	void editingFinished(const QString& text);
};

class ProfileGraphicsView : public QGraphicsView
{
Q_OBJECT
public:
	enum Mode{DIVE, PLAN};

	ProfileGraphicsView(QWidget* parent = 0);
	void plot(struct dive *d, bool forceRedraw = FALSE);
	bool eventFilter(QObject* obj, QEvent* event);
	void clear();
	void setPrintMode(bool mode, bool grayscale = FALSE);

protected:
	void resizeEvent(QResizeEvent *event);
	void mouseMoveEvent(QMouseEvent* event);
	void wheelEvent(QWheelEvent* event);
	void showEvent(QShowEvent* event);

public slots:
	void refresh();
	void edit_dive_time(const QString& time);

private:
	void plot_depth_profile();
	QGraphicsItemGroup *plot_text(text_render_options_t *tro, const QPointF& pos, const QString &text, QGraphicsItem *parent = 0);
	void plot_events(struct divecomputer *dc);
	void plot_one_event(struct event *event);
	void plot_temperature_profile();
	void plot_cylinder_pressure(struct divecomputer *dc);
	void plot_temperature_text();
	void plot_single_temp_text(int sec, int mkelvin);
	void plot_depth_text();
	void plot_text_samples();
	void plot_depth_sample(struct plot_data *entry, text_render_options_t *tro);
	void plot_cylinder_pressure_text();
	void plot_pressure_value(int mbar, int sec, double xalign, double yalign);
	void plot_gas_value(int mbar, int sec, double xalign, double yalign, int o2, int he);
	void plot_deco_text();
	void plot_add_line(int sec, double val, QColor c, QPointF &from);
	void plot_pp_gas_profile();
	void plot_pp_text();
	void plot_depth_scale();

	void create_ruler();
	void add_ruler();
	void remove_ruler();

	QColor getColor(const color_indice_t i);
	QColor get_sac_color(int sac, int avg_sac);
	void scrollViewTo(const QPoint pos);

	QPen defaultPen;
	QBrush defaultBrush;
	ToolTipItem *toolTip;
	graphics_context gc;
	struct dive *dive;
	struct divecomputer *diveDC;
	int zoomLevel;

	bool rulerEnabled;
	bool printMode;
	bool isGrayscale;

	// Top Level Items.
	QGraphicsItem* profileGrid;
	QGraphicsItem* timeMarkers;
	QGraphicsItem* depthMarkers;
	QGraphicsItem* diveComputer;
	RulerItem *rulerItem;

	// For 'Plan' mode.:
	GraphicsTextEditor *depthEditor;
	GraphicsTextEditor *timeEditor;

	enum Mode mode;
};

#endif
