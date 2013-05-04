#ifndef PROFILEGRAPHICS_H
#define PROFILEGRAPHICS_H

#include <QGraphicsView>

class ProfileGraphicsView : public QGraphicsView {
Q_OBJECT
public:
	ProfileGraphicsView(QWidget* parent = 0);
	void plot(struct dive *d);

protected:
	void resizeEvent(QResizeEvent *event);

private:
	void plot_depth_profile(struct graphics_context *gc, struct plot_info *pi);
};

#endif
