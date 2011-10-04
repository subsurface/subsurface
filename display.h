#ifndef DISPLAY_H
#define DISPLAY_H

#include <cairo.h>

extern void repaint_dive(void);
extern void do_print(void);

/*
 * Cairo scaling really is horribly horribly mis-designed.
 *
 * Which is sad, because I really like Cairo otherwise. But
 * the fact that the line width is scaled with the same scale
 * as the coordinate system is a f*&%ing disaster. So we
 * can't use it, and instead have this butt-ugly wrapper thing..
 */
struct graphics_context {
	int printer;
	cairo_t *cr;
	double maxx, maxy;
	double leftx, rightx;
	double topy, bottomy;
};

extern void plot(struct graphics_context *gc, cairo_rectangle_int_t *drawing_area, struct dive *dive);
extern void set_source_rgb(struct graphics_context *gc, double r, double g, double b);
extern void attach_tooltip(int x, int y, int w, int h, const char *text);

#endif
