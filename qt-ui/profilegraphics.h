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

/**!
 *
 * Hookay, so, if you wanna extend the ToolTips that are displayed
 * in the Profile Graph, there's one 'toolTip' widget already on it,
 * you can just pass it to your Reimplementation of QGraphiscItem
 * and do the following:
 *
 * EventItem::setController(ToolTipItem *c)
 * {
 * 	controller = c;
 * }
 *
 * void EventItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
 * {
 *	controller->addToolTip(text, icon);
 * }
 *
 * void EventItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
 * {
 *	controller->removeToolTip(text);
 * }
 *
 * Remember to removeToolTip when you don't want it to be displayed.
 *
 **/
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

private:
	void plot_depth_profile();
	void plot_text(text_render_options_t *tro, double x, double y, const QString &text);
	void plot_events(struct divecomputer *dc);
	void plot_one_event(struct event *event);
	void plot_temperature_profile();
	void plot_cylinder_pressure(struct dive *dive, struct divecomputer *dc);
	void plot_temperature_text();
	void plot_single_temp_text(int sec, int mkelvin);
	void plot_depth_text();
	void plot_text_samples();
	void plot_depth_sample(struct plot_data *entry, text_render_options_t *tro);

	QColor get_sac_color(int sac, int avg_sac);

	QPen defaultPen;
	QBrush defaultBrush;
	ToolTipItem *toolTip;
	graphics_context gc;
};

#endif
