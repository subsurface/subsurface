// SPDX-License-Identifier: GPL-2.0
#ifndef COLOR_H
#define COLOR_H

/* The colors are named by picking the closest match
   from http://chir.ag/projects/name-that-color */

#include <QColor>

static inline QColor makeColor(double r, double g, double b, double a = 1.0)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)  // they are just trolling us with these changes
	return QColor::fromRgbF((float)r, (float)g, (float)b, (float)a);
#else
	return QColor::fromRgbF(r, g, b, a);
#endif
}

// Greens
#define CAMARONE1 makeColor(0.0, 0.4, 0.0)
#define FUNGREEN1 makeColor(0.0, 0.4, 0.2)
#define FUNGREEN1_HIGH_TRANS makeColor(0.0, 0.4, 0.2, 0.25)
#define KILLARNEY1 makeColor(0.2, 0.4, 0.2)
#define APPLE1 makeColor(0.2, 0.6, 0.2)
#define APPLE1_MED_TRANS makeColor(0.2, 0.6, 0.2, 0.5)
#define APPLE1_HIGH_TRANS makeColor(0.2, 0.6, 0.2, 0.25)
#define LIMENADE1 makeColor(0.4, 0.8, 0.0)
#define ATLANTIS1 makeColor(0.4, 0.8, 0.2)
#define ATLANTIS2 makeColor(0.6, 0.8, 0.2)
#define RIOGRANDE1 makeColor(0.8, 0.8, 0.0)
#define EARLSGREEN1 makeColor(0.8, 0.8, 0.2)
#define FORESTGREEN1 makeColor(0.1, 0.5, 0.1)
#define NITROX_GREEN makeColor(0, 0.54, 0.375)

// Reds
#define PERSIANRED1 makeColor(0.8, 0.2, 0.2)
#define TUSCANY1 makeColor(0.8, 0.4, 0.2)
#define PIRATEGOLD1 makeColor(0.8, 0.5, 0.0)
#define PIRATEGOLD1_MED_TRANS makeColor(0.8, 0.5, 0.0, 0.75)
#define HOKEYPOKEY1 makeColor(0.8, 0.6, 0.2)
#define CINNABAR1 makeColor(0.9, 0.3, 0.2)
#define REDORANGE1 makeColor(1.0, 0.2, 0.2)
#define REDORANGE1_HIGH_TRANS makeColor(1.0, 0.2, 0.2, 0.25)
#define REDORANGE1_MED_TRANS makeColor(1.0, 0.2, 0.2, 0.5)
#define RED1_MED_TRANS makeColor(1.0, 0.0, 0.0, 0.5)
#define RED1 makeColor(1.0, 0.0, 0.0)

// Monochromes
#define BLACK1 makeColor(0.0, 0.0, 0.0)
#define BLACK1_LOW_TRANS makeColor(0.0, 0.0, 0.0, 0.75)
#define BLACK1_HIGH_TRANS makeColor(0.0, 0.0, 0.0, 0.25)
#define TUNDORA1_MED_TRANS makeColor(0.3, 0.3, 0.3, 0.5)
#define MED_GRAY_HIGH_TRANS makeColor(0.5, 0.5, 0.5, 0.25)
#define MERCURY1_MED_TRANS makeColor(0.9, 0.9, 0.9, 0.5)
#define CONCRETE1_LOWER_TRANS makeColor(0.95, 0.95, 0.95, 0.9)
#define WHITE1_MED_TRANS makeColor(1.0, 1.0, 1.0, 0.5)
#define WHITE1 makeColor(1.0, 1.0, 1.0)

// Blues
#define GOVERNORBAY2 makeColor(0.2, 0.2, 0.7)
#define GOVERNORBAY1_MED_TRANS makeColor(0.2, 0.2, 0.8, 0.5)
#define ROYALBLUE2 makeColor(0.2, 0.2, 0.9)
#define ROYALBLUE2_LOW_TRANS makeColor(0.2, 0.2, 0.9, 0.75)
#define AIR_BLUE makeColor(0.25, 0.75, 1.0)
#define AIR_BLUE_TRANS makeColor(0.25, 0.75, 1.0, 0.5)

// Yellows / BROWNS
#define SPRINGWOOD1 makeColor(0.95, 0.95, 0.9)
#define SPRINGWOOD1_MED_TRANS makeColor(0.95, 0.95, 0.9, 0.5)
#define BROOM1_LOWER_TRANS makeColor(1.0, 1.0, 0.1, 0.9)
#define PEANUT makeColor(0.5, 0.2, 0.1)
#define PEANUT_MED_TRANS makeColor(0.5, 0.2, 0.1, 0.5)
#define NITROX_YELLOW makeColor(0.98, 0.89, 0.07)

// Magentas
#define MEDIUMREDVIOLET1_HIGHER_TRANS makeColor(0.7, 0.2, 0.7, 0.1)
#define MAGENTA makeColor(1.0, 0.0, 1.0)

#define SAC_COLORS_START_IDX SAC_1
#define SAC_COLORS 9
#define VELOCITY_COLORS_START_IDX VELO_STABLE
#define VELOCITY_COLORS 5

enum color_index_t {
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
	SCR_OCPO2,
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
	DURATION_LINE
};

QColor getColor(const color_index_t i, bool isGrayscale = false);
QColor getSacColor(int sac, int diveSac);
QColor getPressureColor(double density);

#endif // COLOR_H
