#ifndef COLOR_H
#define COLOR_H

/* The colors are named by picking the closest match
   from http://chir.ag/projects/name-that-color */

#include <QColor>
#include <QMap>
#include <QVector>

// Greens
#define CAMARONE1 QColor::fromRgbF(0.0, 0.4, 0.0, 1)
#define FUNGREEN1 QColor::fromRgbF(0.0, 0.4, 0.2, 1)
#define FUNGREEN1_HIGH_TRANS QColor::fromRgbF(0.0, 0.4, 0.2, 0.25)
#define KILLARNEY1 QColor::fromRgbF(0.2, 0.4, 0.2, 1)
#define APPLE1 QColor::fromRgbF(0.2, 0.6, 0.2, 1)
#define APPLE1_MED_TRANS QColor::fromRgbF(0.2, 0.6, 0.2, 0.5)
#define APPLE1_HIGH_TRANS QColor::fromRgbF(0.2, 0.6, 0.2, 0.25)
#define LIMENADE1 QColor::fromRgbF(0.4, 0.8, 0.0, 1)
#define ATLANTIS1 QColor::fromRgbF(0.4, 0.8, 0.2, 1)
#define ATLANTIS2 QColor::fromRgbF(0.6, 0.8, 0.2, 1)
#define RIOGRANDE1 QColor::fromRgbF(0.8, 0.8, 0.0, 1)
#define EARLSGREEN1 QColor::fromRgbF(0.8, 0.8, 0.2, 1)
#define FORESTGREEN1 QColor::fromRgbF(0.1, 0.5, 0.1, 1)
#define NITROX_GREEN QColor::fromRgbF(0, 0.54, 0.375, 1)

// Reds
#define PERSIANRED1 QColor::fromRgbF(0.8, 0.2, 0.2, 1)
#define TUSCANY1 QColor::fromRgbF(0.8, 0.4, 0.2, 1)
#define PIRATEGOLD1 QColor::fromRgbF(0.8, 0.5, 0.0, 1)
#define HOKEYPOKEY1 QColor::fromRgbF(0.8, 0.6, 0.2, 1)
#define CINNABAR1 QColor::fromRgbF(0.9, 0.3, 0.2, 1)
#define REDORANGE1 QColor::fromRgbF(1.0, 0.2, 0.2, 1)
#define REDORANGE1_HIGH_TRANS QColor::fromRgbF(1.0, 0.2, 0.2, 0.25)
#define REDORANGE1_MED_TRANS QColor::fromRgbF(1.0, 0.2, 0.2, 0.5)
#define RED1_MED_TRANS QColor::fromRgbF(1.0, 0.0, 0.0, 0.5)
#define RED1 QColor::fromRgbF(1.0, 0.0, 0.0, 1)

// Monochromes
#define BLACK1 QColor::fromRgbF(0.0, 0.0, 0.0, 1)
#define BLACK1_LOW_TRANS QColor::fromRgbF(0.0, 0.0, 0.0, 0.75)
#define BLACK1_HIGH_TRANS QColor::fromRgbF(0.0, 0.0, 0.0, 0.25)
#define TUNDORA1_MED_TRANS QColor::fromRgbF(0.3, 0.3, 0.3, 0.5)
#define MED_GRAY_HIGH_TRANS QColor::fromRgbF(0.5, 0.5, 0.5, 0.25)
#define MERCURY1_MED_TRANS QColor::fromRgbF(0.9, 0.9, 0.9, 0.5)
#define CONCRETE1_LOWER_TRANS QColor::fromRgbF(0.95, 0.95, 0.95, 0.9)
#define WHITE1_MED_TRANS QColor::fromRgbF(1.0, 1.0, 1.0, 0.5)
#define WHITE1 QColor::fromRgbF(1.0, 1.0, 1.0, 1)

// Blues
#define GOVERNORBAY2 QColor::fromRgbF(0.2, 0.2, 0.7, 1)
#define GOVERNORBAY1_MED_TRANS QColor::fromRgbF(0.2, 0.2, 0.8, 0.5)
#define ROYALBLUE2 QColor::fromRgbF(0.2, 0.2, 0.9, 1)
#define ROYALBLUE2_LOW_TRANS QColor::fromRgbF(0.2, 0.2, 0.9, 0.75)
#define AIR_BLUE QColor::fromRgbF(0.25, 0.75, 1.0, 1)
#define AIR_BLUE_TRANS QColor::fromRgbF(0.25, 0.75, 1.0, 0.5)

// Yellows / BROWNS
#define SPRINGWOOD1 QColor::fromRgbF(0.95, 0.95, 0.9, 1)
#define SPRINGWOOD1_MED_TRANS QColor::fromRgbF(0.95, 0.95, 0.9, 0.5)
#define BROOM1_LOWER_TRANS QColor::fromRgbF(1.0, 1.0, 0.1, 0.9)
#define PEANUT QColor::fromRgbF(0.5, 0.2, 0.1, 1.0)
#define PEANUT_MED_TRANS QColor::fromRgbF(0.5, 0.2, 0.1, 0.5)
#define NITROX_YELLOW QColor::fromRgbF(0.98, 0.89, 0.07, 1.0)

// Magentas
#define MEDIUMREDVIOLET1_HIGHER_TRANS QColor::fromRgbF(0.7, 0.2, 0.7, 0.1)

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
	O2SETPOINT,
	CCRSENSOR1,
	CCRSENSOR2,
	CCRSENSOR3,
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

#endif // COLOR_H
