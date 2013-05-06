#ifndef PROFILEGRAPHICS_H
#define PROFILEGRAPHICS_H

#include <QGraphicsView>

struct text_render_options;
struct graphics_context;
struct plot_info;
typedef struct text_render_options text_render_options_t;

class ProfileGraphicsView : public QGraphicsView {
Q_OBJECT
public:
	ProfileGraphicsView(QWidget* parent = 0);
	void plot(struct dive *d);

protected:
	void resizeEvent(QResizeEvent *event);

private:
	void plot_depth_profile(struct graphics_context *gc, struct plot_info *pi);
	void plot_text(struct graphics_context *gc, text_render_options_t *tro, double x, double y, const QString &text);
	void plot_events(struct graphics_context *gc, struct plot_info *pi, struct divecomputer *dc);
	void plot_one_event(struct graphics_context *gc, struct plot_info *pi, struct event *event);

	QPen defaultPen;
	QBrush defaultBrush;
};

#endif
