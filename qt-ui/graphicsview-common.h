#ifndef GRAPHICSVIEW_COMMON_H
#define GRAPHICSVIEW_COMMON_H

#include "../color.h"
#include <QMap>
#include <QVector>
#include <QColor>

#define SAC_COLORS_START_IDX SAC_1
#define SAC_COLORS 9
#define VELOCITY_COLORS_START_IDX VELO_STABLE
#define VELOCITY_COLORS 5

typedef enum {
	/* SAC colors. Order is important, the SAC_COLORS_START_IDX define above. */
	SAC_1,
	SAC_2,
	SAC_3,
	SAC_4,
	SAC_5,
	SAC_6,
	SAC_7,
	SAC_8,
	SAC_9,

	/* Velocity colors.  Order is still important, ref VELOCITY_COLORS_START_IDX. */
	VELO_STABLE,
	VELO_SLOW,
	VELO_MODERATE,
	VELO_FAST,
	VELO_CRAZY,

	/* gas colors */
	PO2,
	PO2_ALERT,
	PN2,
	PN2_ALERT,
	PHE,
	PHE_ALERT,
	PP_LINES,

	/* Other colors */
	TEXT_BACKGROUND,
	ALERT_BG,
	ALERT_FG,
	EVENTS,
	SAMPLE_DEEP,
	SAMPLE_SHALLOW,
	SMOOTHED,
	MINUTE,
	TIME_GRID,
	TIME_TEXT,
	DEPTH_GRID,
	MEAN_DEPTH,
	HR_TEXT,
	HR_PLOT,
	HR_AXIS,
	DEPTH_TOP,
	DEPTH_BOTTOM,
	TEMP_TEXT,
	TEMP_PLOT,
	SAC_DEFAULT,
	BOUNDING_BOX,
	PRESSURE_TEXT,
	BACKGROUND,
	BACKGROUND_TRANS,
	CEILING_SHALLOW,
	CEILING_DEEP,
	CALC_CEILING_SHALLOW,
	CALC_CEILING_DEEP,
	TISSUE_PERCENTAGE,
	GF_LINE,
	AMB_PRESSURE_LINE
} color_indice_t;


/* profile_color[color indice] = COLOR(screen color, b/w printer color, color printer}} printer & screen colours could be different */

extern QMap<color_indice_t, QVector<QColor> > profile_color;
void fill_profile_color();
QColor getColor(const color_indice_t i, bool isGrayscale = false);
QColor getSacColor(int sac, int diveSac);
struct text_render_options {
	double size;
	color_indice_t color;
	double hpos, vpos;
};

typedef text_render_options text_render_options_t;
#endif // GRAPHICSVIEW_COMMON_H
