#ifndef PROFILEGRAPHICS_H
#define PROFILEGRAPHICS_H

#include <QGraphicsView>
#include <QGraphicsItem>
#include <QIcon>

struct text_render_options;
struct graphics_context;
struct plot_info;
typedef struct text_render_options text_render_options_t;


class ToolTipItem;
class ToolTipStatusHandler;

class ToolTipStatusHandler :public QObject, public QGraphicsEllipseItem {
public:
    explicit ToolTipStatusHandler(QObject* parent = 0);
protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event);
};

class ToolTipItem :public QObject, public QGraphicsPathItem {
	Q_OBJECT
	Q_PROPERTY(QRectF rect READ boundingRect WRITE setRect)

public:
	enum Status {COLLAPSED, EXPANDED};
	enum {ICON_SMALL = 16, ICON_MEDIUM = 24, ICON_BIG = 32};

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
	enum Status status;
	QMap<QString, ToolTip > toolTips;
	QGraphicsRectItem *background;
	QRectF rectangle;
};

class ProfileGraphicsView : public QGraphicsView {
Q_OBJECT
public:
	ProfileGraphicsView(QWidget* parent = 0);
	void plot(struct dive *d);
	void addToolTip(const QString& text, const QIcon& icon = QIcon());
	void removeToolTip(const QString& text);

protected:
	void resizeEvent(QResizeEvent *event);

private:
	void plot_depth_profile(struct graphics_context *gc, struct plot_info *pi);
	void plot_text(struct graphics_context *gc, text_render_options_t *tro, double x, double y, const QString &text);
	void plot_events(struct graphics_context *gc, struct plot_info *pi, struct divecomputer *dc);
	void plot_one_event(struct graphics_context *gc, struct plot_info *pi, struct event *event);

	QPen defaultPen;
	QBrush defaultBrush;
	ToolTipItem *toolTip;
};


#endif
