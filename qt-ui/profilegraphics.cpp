#include "profilegraphics.h"

#include <QGraphicsScene>
#include <QResizeEvent>

#include <QDebug>

#include "../color.h"

#define SAC_COLORS_START_IDX SAC_1
#define SAC_COLORS 9
#define VELOCITY_COLORS_START_IDX VELO_STABLE
#define VELOCITY_COLORS 5

typedef enum {
	/* SAC colors. Order is important, the SAC_COLORS_START_IDX define above. */
	SAC_1, SAC_2, SAC_3, SAC_4, SAC_5, SAC_6, SAC_7, SAC_8, SAC_9,

	/* Velocity colors.  Order is still important, ref VELOCITY_COLORS_START_IDX. */
	VELO_STABLE, VELO_SLOW, VELO_MODERATE, VELO_FAST, VELO_CRAZY,

	/* gas colors */
	PO2, PO2_ALERT, PN2, PN2_ALERT, PHE, PHE_ALERT, PP_LINES,

	/* Other colors */
	TEXT_BACKGROUND, ALERT_BG, ALERT_FG, EVENTS, SAMPLE_DEEP, SAMPLE_SHALLOW,
	SMOOTHED, MINUTE, TIME_GRID, TIME_TEXT, DEPTH_GRID, MEAN_DEPTH, DEPTH_TOP,
	DEPTH_BOTTOM, TEMP_TEXT, TEMP_PLOT, SAC_DEFAULT, BOUNDING_BOX, PRESSURE_TEXT, BACKGROUND,
	CEILING_SHALLOW, CEILING_DEEP, CALC_CEILING_SHALLOW, CALC_CEILING_DEEP
} color_indice_t;

/* profile_color[color indice] = COLOR(screen color, b/w printer color, color printer}} printer & screen colours could be different */
QMap<color_indice_t, QVector<QColor> > profile_color;
void fill_profile_color()
{
#define COLOR(x, y, z) QVector<QColor>() << x << y << z;
	profile_color[SAC_1]           = COLOR(FUNGREEN1, BLACK1_LOW_TRANS, FUNGREEN1);
	profile_color[SAC_2]           = COLOR(APPLE1, BLACK1_LOW_TRANS, APPLE1);
	profile_color[SAC_3]           = COLOR(ATLANTIS1, BLACK1_LOW_TRANS, ATLANTIS1);
	profile_color[SAC_4]           = COLOR(ATLANTIS2, BLACK1_LOW_TRANS, ATLANTIS2);
	profile_color[SAC_5]           = COLOR(EARLSGREEN1, BLACK1_LOW_TRANS, EARLSGREEN1);
	profile_color[SAC_6]           = COLOR(HOKEYPOKEY1, BLACK1_LOW_TRANS, HOKEYPOKEY1);
	profile_color[SAC_7]           = COLOR(TUSCANY1, BLACK1_LOW_TRANS, TUSCANY1);
	profile_color[SAC_8]           = COLOR(CINNABAR1, BLACK1_LOW_TRANS, CINNABAR1);
	profile_color[SAC_9]           = COLOR(REDORANGE1, BLACK1_LOW_TRANS, REDORANGE1);

	profile_color[VELO_STABLE]     = COLOR(CAMARONE1, BLACK1_LOW_TRANS, CAMARONE1);
	profile_color[VELO_SLOW]       = COLOR(LIMENADE1, BLACK1_LOW_TRANS, LIMENADE1);
	profile_color[VELO_MODERATE]   = COLOR(RIOGRANDE1, BLACK1_LOW_TRANS, RIOGRANDE1);
	profile_color[VELO_FAST]       = COLOR(PIRATEGOLD1, BLACK1_LOW_TRANS, PIRATEGOLD1);
	profile_color[VELO_CRAZY]      = COLOR(RED1, BLACK1_LOW_TRANS, RED1);

	profile_color[PO2]             = COLOR(APPLE1, BLACK1_LOW_TRANS, APPLE1);
	profile_color[PO2_ALERT]       = COLOR(RED1, BLACK1_LOW_TRANS, RED1);
	profile_color[PN2]             = COLOR(BLACK1_LOW_TRANS, BLACK1_LOW_TRANS, BLACK1_LOW_TRANS);
	profile_color[PN2_ALERT]       = COLOR(RED1, BLACK1_LOW_TRANS, RED1);
	profile_color[PHE]             = COLOR(PEANUT, BLACK1_LOW_TRANS, PEANUT);
	profile_color[PHE_ALERT]       = COLOR(RED1, BLACK1_LOW_TRANS, RED1);
	profile_color[PP_LINES]        = COLOR(BLACK1_HIGH_TRANS, BLACK1_HIGH_TRANS, BLACK1_HIGH_TRANS);

	profile_color[TEXT_BACKGROUND] = COLOR(CONCRETE1_LOWER_TRANS, WHITE1, CONCRETE1_LOWER_TRANS);
	profile_color[ALERT_BG]        = COLOR(BROOM1_LOWER_TRANS, BLACK1_LOW_TRANS, BROOM1_LOWER_TRANS);
	profile_color[ALERT_FG]        = COLOR(BLACK1_LOW_TRANS, BLACK1_LOW_TRANS, BLACK1_LOW_TRANS);
	profile_color[EVENTS]          = COLOR(REDORANGE1, BLACK1_LOW_TRANS, REDORANGE1);
	profile_color[SAMPLE_DEEP]     = COLOR(PERSIANRED1, BLACK1_LOW_TRANS, PERSIANRED1);
	profile_color[SAMPLE_SHALLOW]  = COLOR(PERSIANRED1, BLACK1_LOW_TRANS, PERSIANRED1);
	profile_color[SMOOTHED]        = COLOR(REDORANGE1_HIGH_TRANS, BLACK1_LOW_TRANS, REDORANGE1_HIGH_TRANS);
	profile_color[MINUTE]          = COLOR(MEDIUMREDVIOLET1_HIGHER_TRANS, BLACK1_LOW_TRANS, MEDIUMREDVIOLET1_HIGHER_TRANS);
	profile_color[TIME_GRID]       = COLOR(WHITE1, BLACK1_HIGH_TRANS, TUNDORA1_MED_TRANS);
	profile_color[TIME_TEXT]       = COLOR(FORESTGREEN1, BLACK1_LOW_TRANS, FORESTGREEN1);
	profile_color[DEPTH_GRID]      = COLOR(WHITE1, BLACK1_HIGH_TRANS, TUNDORA1_MED_TRANS);
	profile_color[MEAN_DEPTH]      = COLOR(REDORANGE1_MED_TRANS, BLACK1_LOW_TRANS, REDORANGE1_MED_TRANS);
	profile_color[DEPTH_BOTTOM]    = COLOR(GOVERNORBAY1_MED_TRANS, BLACK1_HIGH_TRANS, GOVERNORBAY1_MED_TRANS);
	profile_color[DEPTH_TOP]       = COLOR(MERCURY1_MED_TRANS, WHITE1_MED_TRANS, MERCURY1_MED_TRANS);
	profile_color[TEMP_TEXT]       = COLOR(GOVERNORBAY2, BLACK1_LOW_TRANS, GOVERNORBAY2);
	profile_color[TEMP_PLOT]       = COLOR(ROYALBLUE2_LOW_TRANS, BLACK1_LOW_TRANS, ROYALBLUE2_LOW_TRANS);
	profile_color[SAC_DEFAULT]     = COLOR(WHITE1, BLACK1_LOW_TRANS, FORESTGREEN1);
	profile_color[BOUNDING_BOX]    = COLOR(WHITE1, BLACK1_LOW_TRANS, TUNDORA1_MED_TRANS);
	profile_color[PRESSURE_TEXT]   = COLOR(KILLARNEY1, BLACK1_LOW_TRANS, KILLARNEY1);
	profile_color[BACKGROUND]      = COLOR(SPRINGWOOD1, BLACK1_LOW_TRANS, SPRINGWOOD1);
	profile_color[CEILING_SHALLOW] = COLOR(REDORANGE1_HIGH_TRANS, BLACK1_HIGH_TRANS, REDORANGE1_HIGH_TRANS);
	profile_color[CEILING_DEEP]    = COLOR(RED1_MED_TRANS, BLACK1_HIGH_TRANS, RED1_MED_TRANS);
	profile_color[CALC_CEILING_SHALLOW] = COLOR(FUNGREEN1_HIGH_TRANS, BLACK1_HIGH_TRANS, FUNGREEN1_HIGH_TRANS);
	profile_color[CALC_CEILING_DEEP]    = COLOR(APPLE1_HIGH_TRANS, BLACK1_HIGH_TRANS, APPLE1_HIGH_TRANS);
#undef COLOR
}

ProfileGraphicsView::ProfileGraphicsView(QWidget* parent) : QGraphicsView(parent)
{
	setScene(new QGraphicsScene());
	scene()->setSceneRect(0,0,100,100);
	fill_profile_color();
}

void ProfileGraphicsView::plot(struct dive *d)
{
	qDebug() << "Start the plotting of the dive here.";
}

void ProfileGraphicsView::resizeEvent(QResizeEvent *event)
{
	// Fits the scene's rectangle on the view.
	// I can pass some parameters to this -
	// like Qt::IgnoreAspectRatio or Qt::KeepAspectRatio
	QRectF r = scene()->sceneRect();
	fitInView ( r.x() - 2, r.y() -2, r.width() + 4, r.height() + 4); // do a little bit of spacing;
}
