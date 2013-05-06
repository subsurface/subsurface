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

	QPen defaultPen;
	QBrush defaultBrush;
};

#endif
