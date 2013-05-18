#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#define SCALE_SCREEN 1.0
#define SCALE_PRINT (1.0 / get_screen_dpi())

extern void repaint_dive(void);
extern void do_print(void);

// Commented out because I don't know how to get the dpi on a paint device yet.
extern double get_screen_dpi(void);

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
	bool has_ndl;
	struct plot_data *entry;
};

/*
// I'm not sure if this is needed anymore - but keeping this here
// so I wont break stuff trying to redo the well.
*/

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
	double maxx, maxy;
	double leftx, rightx;
	double topy, bottomy;
	unsigned int maxtime;
	struct plot_info pi;
};

typedef enum { SC_SCREEN, SC_PRINT } scale_mode_t;

extern void plot(struct graphics_context *gc, struct dive *dive, scale_mode_t scale);
extern struct divecomputer *select_dc(struct divecomputer *main);
extern void init_profile_background(struct graphics_context *gc);
extern void attach_tooltip(int x, int y, int w, int h, const char *text, struct event *event);
extern void get_plot_details(struct graphics_context *gc, int time, char *buf, int bufsize);
extern int x_to_time(double x);
extern int x_abs(double x);

struct options {
	enum { PRETTY, TABLE, TWOPERPAGE } type;
	int print_selected;
	int color_selected;
	bool notes_up;
	int profile_height, notes_height, tanks_height;
};

extern char zoomed_plot, dc_number;

extern unsigned int amount_selected;

extern int is_default_dive_computer_device(const char *);
extern int is_default_dive_computer(const char *, const char *);
extern const char *default_dive_computer_vendor;
extern const char *default_dive_computer_product;
extern const char *default_dive_computer_device;

#ifdef __cplusplus
}
#endif

#endif
