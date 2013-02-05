#ifndef DISPLAY_H
#define DISPLAY_H

#include <cairo.h>

#define SCALE_SCREEN 1.0
#define SCALE_PRINT (1.0 / get_screen_dpi())

extern void repaint_dive(void);
extern void do_print(void);
extern gdouble get_screen_dpi(void);

/* Plot info with smoothing, velocity indication
 * and one-, two- and three-minute minimums and maximums */
struct plot_info {
	int nr;
	int maxtime;
	int meandepth, maxdepth;
	int minpressure, maxpressure;
	int mintemp, maxtemp;
	double endtempcoord;
	double maxpp;
	gboolean has_ndl;
	struct plot_data *entry;
};

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
	cairo_rectangle_t drawing_area;
	double maxx, maxy;
	double leftx, rightx;
	double topy, bottomy;
	unsigned int maxtime;
	struct plot_info pi;
};

typedef enum { SC_SCREEN, SC_PRINT } scale_mode_t;

extern void plot(struct graphics_context *gc, struct dive *dive, scale_mode_t scale);
extern void init_profile_background(struct graphics_context *gc);
extern void attach_tooltip(int x, int y, int w, int h, const char *text);
extern void get_plot_details(struct graphics_context *gc, int time, char *buf, size_t bufsize);

struct options {
	enum { PRETTY, TABLE, TWOPERPAGE } type;
	int print_selected;
};

extern char zoomed_plot, dc_number;

#endif
