#ifndef PROFILEGRAPHICS_H
#define PROFILEGRAPHICS_H

#include "../display.h"
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QIcon>

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

	void collapse();
	void expand();
	void clear();
	void addToolTip(const QString& toolTip, const QIcon& icon = QIcon());
	void removeToolTip(const QString& toolTip);
	void refresh(struct graphics_context* gc, QPointF pos);

public Q_SLOTS:
	void setRect(const QRectF& rect);

private:
	typedef QPair<QGraphicsPixmapItem*, QGraphicsSimpleTextItem*> ToolTip;
	QMap<QString, ToolTip > toolTips;
	QGraphicsPathItem *background;
	QGraphicsLineItem *separator;
	QGraphicsSimpleTextItem *title;

	QRectF rectangle;
};

class EventItem : public QGraphicsPolygonItem
{
public:
	explicit EventItem(QGraphicsItem* parent = 0);

private:
	ToolTipItem *controller;
	QString text;
	QIcon icon;
};

class ProfileGraphicsView : public QGraphicsView
{
Q_OBJECT
public:
	ProfileGraphicsView(QWidget* parent = 0);
	void plot(struct dive *d);
	bool eventFilter(QObject* obj, QEvent* event);

protected:
	void resizeEvent(QResizeEvent *event);
    void mouseMoveEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);
    void showEvent(QShowEvent* event);

private:
	void plot_depth_profile();
	QGraphicsSimpleTextItem* plot_text(text_render_options_t *tro, const QPointF& pos, const QString &text, QGraphicsItem *parent = 0);
	void plot_events(struct divecomputer *dc);
	void plot_one_event(struct event *event);
	void plot_temperature_profile();
	void plot_cylinder_pressure(struct dive *dive, struct divecomputer *dc);
	void plot_temperature_text();
	void plot_single_temp_text(int sec, int mkelvin);
	void plot_depth_text();
	void plot_text_samples();
	void plot_depth_sample(struct plot_data *entry, text_render_options_t *tro);
	void plot_cylinder_pressure_text();
	void plot_pressure_value(int mbar, int sec, double xalign, double yalign);
	void plot_deco_text();
	void plot_pp_gas_profile();
	void plot_pp_text();
	void plot_depth_scale();

	QColor get_sac_color(int sac, int avg_sac);

	QPen defaultPen;
	QBrush defaultBrush;
	ToolTipItem *toolTip;
	graphics_context gc;
	struct dive *dive;
	int zoomLevel;

	// Top Level Items.
	QGraphicsItem* profileGrid;
	QGraphicsItem* timeMarkers;
	QGraphicsItem* depthMarkers;
	QGraphicsItem* diveComputer;
};

#endif
