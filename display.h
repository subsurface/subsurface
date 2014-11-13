#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

struct membuffer;

#define SCALE_SCREEN 1.0
#define SCALE_PRINT (1.0 / get_screen_dpi())

extern double get_screen_dpi(void);

/* Plot info with smoothing, velocity indication
 * and one-, two- and three-minute minimums and maximums */
struct plot_info {
	int nr;
	int maxtime;
	int meandepth, maxdepth;
	int minpressure, maxpressure;
	int minhr, maxhr;
	int mintemp, maxtemp;
	enum {AIR, NITROX, TRIMIX} dive_type;
	double endtempcoord;
	double maxpp;
	bool has_ndl;
	struct plot_data *entry;
};

typedef enum {
	SC_SCREEN,
	SC_PRINT
} scale_mode_t;

extern struct divecomputer *select_dc(struct dive *);

struct print_options {
	enum print_type {
		PRETTY,
		TABLE,
		TWOPERPAGE,
		ONEPERPAGE
	} type;
	bool print_selected;
	bool color_selected;
	bool notes_up;
	bool landscape;
	int margins[4]; // left, top, right, bottom
};

extern unsigned int dc_number;

extern unsigned int amount_selected;

extern int is_default_dive_computer_device(const char *);
extern int is_default_dive_computer(const char *, const char *);

typedef void (*device_callback_t)(const char *name, void *userdata);

#define DC_TYPE_SERIAL 1
#define DC_TYPE_UEMIS 2
#define DC_TYPE_OTHER 3

int enumerate_devices(device_callback_t callback, void *userdata, int dc_type);

extern const char *default_dive_computer_vendor;
extern const char *default_dive_computer_product;
extern const char *default_dive_computer_device;

#define AMB_PERCENTAGE 50.0

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
