// SPDX-License-Identifier: GPL-2.0
#include "color.h"
#include <QMap>
#include <array>

// Note that std::array<QColor, 2> is in every respect equivalent to QColor[2],
// but allows assignment, comparison, can be returned from functions, etc.
static QMap<color_index_t, std::array<QColor, 2>> profile_color = {
	{ SAC_1, {{ FUNGREEN1, BLACK1_LOW_TRANS }} },
	{ SAC_2, {{ APPLE1, BLACK1_LOW_TRANS }} },
	{ SAC_3, {{ ATLANTIS1, BLACK1_LOW_TRANS }} },
	{ SAC_4, {{ ATLANTIS2, BLACK1_LOW_TRANS }} },
	{ SAC_5, {{ EARLSGREEN1, BLACK1_LOW_TRANS }} },
	{ SAC_6, {{ HOKEYPOKEY1, BLACK1_LOW_TRANS }} },
	{ SAC_7, {{ TUSCANY1, BLACK1_LOW_TRANS }} },
	{ SAC_8, {{ CINNABAR1, BLACK1_LOW_TRANS }} },
	{ SAC_9, {{ REDORANGE1, BLACK1_LOW_TRANS }} },

	{ VELO_STABLE, {{ CAMARONE1, BLACK1_LOW_TRANS }} },
	{ VELO_SLOW, {{ LIMENADE1, BLACK1_LOW_TRANS }} },
	{ VELO_MODERATE, {{ RIOGRANDE1, BLACK1_LOW_TRANS }} },
	{ VELO_FAST, {{ PIRATEGOLD1, BLACK1_LOW_TRANS }} },
	{ VELO_CRAZY, {{ RED1, BLACK1_LOW_TRANS }} },

	{ PO2, {{ APPLE1, BLACK1_LOW_TRANS }} },
	{ PO2_ALERT, {{ RED1, BLACK1_LOW_TRANS }} },
	{ PN2, {{ BLACK1_LOW_TRANS, BLACK1_LOW_TRANS }} },
	{ PN2_ALERT, {{ RED1, BLACK1_LOW_TRANS }} },
	{ PHE, {{ PEANUT, BLACK1_LOW_TRANS }} },
	{ PHE_ALERT, {{ RED1, BLACK1_LOW_TRANS }} },
	{ O2SETPOINT, {{ PIRATEGOLD1_MED_TRANS, BLACK1_LOW_TRANS }} },
	{ CCRSENSOR1, {{ TUNDORA1_MED_TRANS, BLACK1_LOW_TRANS }} },
	{ CCRSENSOR2, {{ ROYALBLUE2_LOW_TRANS, BLACK1_LOW_TRANS }} },
	{ CCRSENSOR3, {{ PEANUT, BLACK1_LOW_TRANS }} },
	{ SCR_OCPO2, {{ PIRATEGOLD1_MED_TRANS, BLACK1_LOW_TRANS }} },

	{ PP_LINES, {{ BLACK1_HIGH_TRANS, BLACK1_LOW_TRANS }} },

	{ TEXT_BACKGROUND, {{ CONCRETE1_LOWER_TRANS, WHITE1 }} },
	{ ALERT_BG, {{ BROOM1_LOWER_TRANS, BLACK1_LOW_TRANS }} },
	{ ALERT_FG, {{ BLACK1_LOW_TRANS, WHITE1 }} },
	{ EVENTS, {{ REDORANGE1, BLACK1_LOW_TRANS }} },
	{ SAMPLE_DEEP, {{ QColor(Qt::red).darker(), BLACK1 }} },
	{ SAMPLE_SHALLOW, {{ QColor(Qt::red).lighter(), BLACK1_LOW_TRANS }} },
	{ SMOOTHED, {{ REDORANGE1_HIGH_TRANS, BLACK1_LOW_TRANS }} },
	{ MINUTE, {{ MEDIUMREDVIOLET1_HIGHER_TRANS, BLACK1_LOW_TRANS }} },
	{ TIME_GRID, {{ WHITE1, BLACK1_HIGH_TRANS }} },
	{ TIME_TEXT, {{ FORESTGREEN1, BLACK1 }} },
	{ DEPTH_GRID, {{ WHITE1, BLACK1_HIGH_TRANS }} },
	{ MEAN_DEPTH, {{ REDORANGE1_MED_TRANS, BLACK1_LOW_TRANS }} },
	{ HR_PLOT, {{ REDORANGE1_MED_TRANS, BLACK1_LOW_TRANS }} },
	{ HR_TEXT, {{ REDORANGE1_MED_TRANS, BLACK1_LOW_TRANS }} },
	{ HR_AXIS, {{ MED_GRAY_HIGH_TRANS, MED_GRAY_HIGH_TRANS }} },
	{ DEPTH_BOTTOM, {{ GOVERNORBAY1_MED_TRANS, BLACK1_HIGH_TRANS }} },
	{ DEPTH_TOP, {{ MERCURY1_MED_TRANS, WHITE1_MED_TRANS }} },
	{ TEMP_TEXT, {{ GOVERNORBAY2, BLACK1_LOW_TRANS }} },
	{ TEMP_PLOT, {{ ROYALBLUE2_LOW_TRANS, BLACK1_LOW_TRANS }} },
	{ SAC_DEFAULT, {{ WHITE1, BLACK1_LOW_TRANS }} },
	{ BOUNDING_BOX, {{ WHITE1, BLACK1_LOW_TRANS }} },
	{ PRESSURE_TEXT, {{ KILLARNEY1, BLACK1_LOW_TRANS }} },
	{ BACKGROUND, {{ SPRINGWOOD1, WHITE1 }} },
	{ BACKGROUND_TRANS, {{ SPRINGWOOD1_MED_TRANS, WHITE1_MED_TRANS }} },
	{ CEILING_SHALLOW, {{ REDORANGE1_HIGH_TRANS, BLACK1_HIGH_TRANS }} },
	{ CEILING_DEEP, {{ RED1_MED_TRANS, BLACK1_HIGH_TRANS }} },
	{ CALC_CEILING_SHALLOW, {{ FUNGREEN1_HIGH_TRANS, BLACK1_HIGH_TRANS }} },
	{ CALC_CEILING_DEEP, {{ APPLE1_HIGH_TRANS, BLACK1_HIGH_TRANS }} },
	{ TISSUE_PERCENTAGE, {{ GOVERNORBAY2, BLACK1_LOW_TRANS }} },
	{ DURATION_LINE, {{ BLACK1, BLACK1_LOW_TRANS }} }
};

QColor getColor(const color_index_t i, bool isGrayscale)
{
	if (profile_color.count() > i && i >= 0)
		return profile_color[i][isGrayscale ? 1 : 0];
	return QColor(Qt::black);
}

QColor getSacColor(int sac, int avg_sac)
{
	int sac_index = 0;
	int delta = sac - avg_sac + 7000;

	sac_index = delta / 2000;
	if (sac_index < 0)
		sac_index = 0;
	if (sac_index > SAC_COLORS - 1)
		sac_index = SAC_COLORS - 1;
	return getColor((color_index_t)(SAC_COLORS_START_IDX + sac_index), false);
}

QColor getPressureColor(double density)
{
	QColor color;

	int h = ((int) (180.0 - 180.0 * density / 8.0));
	while (h < 0)
		h += 360;
	color.setHsv(h , 255, 255);
	return color;
}
