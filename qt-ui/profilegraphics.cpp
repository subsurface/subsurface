#include "profilegraphics.h"

#include <QGraphicsScene>
#include <QResizeEvent>
#include <QGraphicsLineItem>
#include <QPen>
#include <QBrush>
#include <QDebug>

#include "../color.h"
#include "../display.h"
#include "../dive.h"
#include "../profile.h"

#define SAC_COLORS_START_IDX SAC_1
#define SAC_COLORS 9
#define VELOCITY_COLORS_START_IDX VELO_STABLE
#define VELOCITY_COLORS 5

/* Scale to 0,0 -> maxx,maxy */
#define SCALEX(gc,x)  (((x)-gc->leftx)/(gc->rightx-gc->leftx)*gc->maxx)
#define SCALEY(gc,y)  (((y)-gc->topy)/(gc->bottomy-gc->topy)*gc->maxy)
#define SCALE(gc,x,y) SCALEX(gc,x),SCALEY(gc,y)

static struct graphics_context last_gc;
static double plot_scale = SCALE_SCREEN;

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


#define COLOR(x, y, z) QVector<QColor>() << x << y << z;
/* profile_color[color indice] = COLOR(screen color, b/w printer color, color printer}} printer & screen colours could be different */
QMap<color_indice_t, QVector<QColor> > profile_color;
void fill_profile_color()
{
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

}
#undef COLOR

struct text_render_options{
	double size;
	color_indice_t color;
	double hpos, vpos;
};

ProfileGraphicsView::ProfileGraphicsView(QWidget* parent) : QGraphicsView(parent)
{
	setScene(new QGraphicsScene());
	setBackgroundBrush(QColor("#F3F3E6"));
	scene()->setSceneRect(0,0,1000,1000);

	setRenderHint(QPainter::Antialiasing);
	setRenderHint(QPainter::HighQualityAntialiasing);
	setRenderHint(QPainter::SmoothPixmapTransform);

	fill_profile_color();
}

static void plot_set_scale(scale_mode_t scale)
{
	switch (scale) {
	default:
	case SC_SCREEN:
		plot_scale = SCALE_SCREEN;
		break;
	case SC_PRINT:
		plot_scale = SCALE_PRINT;
		break;
	}
}

void ProfileGraphicsView::plot(struct dive *dive)
{
	// Clear the items before drawing this dive.
	qDeleteAll(scene()->items());
	scene()->clear();

	if(!dive)
		return;

	struct plot_info *pi;
	struct divecomputer *dc = &dive->dc;

	// This was passed around in the Cairo version / needed?
	graphics_context gc;
	const char *nickname;

	// Fix this for printing / screen later.
	// plot_set_scale( scale_mode_t);

	if (!dc->samples) {
		static struct sample fake[4];
		static struct divecomputer fakedc;
		fakedc = dive->dc;
		fakedc.sample = fake;
		fakedc.samples = 4;

		/* The dive has no samples, so create a few fake ones.  This assumes an
		ascent/descent rate of 9 m/min, which is just below the limit for FAST. */
		int duration = dive->dc.duration.seconds;
		int maxdepth = dive->dc.maxdepth.mm;
		int asc_desc_time = dive->dc.maxdepth.mm*60/9000;
		if (asc_desc_time * 2 >= duration)
			asc_desc_time = duration / 2;
		fake[1].time.seconds = asc_desc_time;
		fake[1].depth.mm = maxdepth;
		fake[2].time.seconds = duration - asc_desc_time;
		fake[2].depth.mm = maxdepth;
		fake[3].time.seconds = duration * 1.00;
		fakedc.events = dc->events;
		dc = &fakedc;
	}

	/*
	 * Set up limits that are independent of
	 * the dive computer
	 */
	calculate_max_limits(dive, dc, &gc);

	/*
	 * We don't use "cairo_translate()" because that doesn't
	 * scale line width etc. But the actual scaling we need
	 * do set up ourselves..
	 *
	 * Snif. What a pity.
	 */
	QRectF drawing_area = scene()->sceneRect();
	gc.maxx = (drawing_area.width() - 2 * drawing_area.x());
	gc.maxy = (drawing_area.height() - 2 * drawing_area.y());

	dc = select_dc(dc);

	/* This is per-dive-computer. Right now we just do the first one */
	pi = create_plot_info(dive, dc, &gc);

	/* Depth profile */
	plot_depth_profile(&gc, pi);
#if 0
	plot_events(gc, pi, dc);

	/* Temperature profile */
	plot_temperature_profile(gc, pi);

	/* Cylinder pressure plot */
	plot_cylinder_pressure(gc, pi, dive, dc);

	/* Text on top of all graphs.. */
	plot_temperature_text(gc, pi);
	plot_depth_text(gc, pi);
	plot_cylinder_pressure_text(gc, pi);
	plot_deco_text(gc, pi);

	/* Bounding box last */
	gc->leftx = 0; gc->rightx = 1.0;
	gc->topy = 0; gc->bottomy = 1.0;

	set_source_rgba(gc, BOUNDING_BOX);
	cairo_set_line_width_scaled(gc->cr, 1);
	move_to(gc, 0, 0);
	line_to(gc, 0, 1);
	line_to(gc, 1, 1);
	line_to(gc, 1, 0);
	cairo_close_path(gc->cr);
	cairo_stroke(gc->cr);

	/* Put the dive computer name in the lower left corner */
	nickname = get_dc_nickname(dc->model, dc->deviceid);
	if (!nickname || *nickname == '\0')
		nickname = dc->model;
	if (nickname) {
		static const text_render_options_t computer = {DC_TEXT_SIZE, TIME_TEXT, LEFT, MIDDLE};
		plot_text(gc, &computer, 0, 1, "%s", nickname);
	}

	if (PP_GRAPHS_ENABLED) {
		plot_pp_gas_profile(gc, pi);
		plot_pp_text(gc, pi);
	}

	/* now shift the translation back by half the margin;
	 * this way we can draw the vertical scales on both sides */
	cairo_translate(gc->cr, -drawing_area->x / 2.0, 0);
	gc->maxx += drawing_area->x;
	gc->leftx = -(drawing_area->x / drawing_area->width) / 2.0;
	gc->rightx = 1.0 - gc->leftx;

	plot_depth_scale(gc, pi);

	if (gc->printer) {
		free(pi->entry);
		last_pi_entry = pi->entry = NULL;
		pi->nr = 0;
	}
#endif
}


void ProfileGraphicsView::plot_depth_profile(struct graphics_context *gc, struct plot_info *pi)
{
	int i, incr;
	int sec, depth;
	struct plot_data *entry;
	int maxtime, maxdepth, marker, maxline;
	int increments[8] = { 10, 20, 30, 60, 5*60, 10*60, 15*60, 30*60 };

	/* Get plot scaling limits */
	maxtime = get_maxtime(pi);
	maxdepth = get_maxdepth(pi);

	gc->maxtime = maxtime;

	/* Time markers: at most every 10 seconds, but no more than 12 markers.
	 * We start out with 10 seconds and increment up to 30 minutes,
	 * depending on the dive time.
	 * This allows for 6h dives - enough (I hope) for even the craziest
	 * divers - but just in case, for those 8h depth-record-breaking dives,
	 * we double the interval if this still doesn't get us to 12 or fewer
	 * time markers */
	i = 0;
	while (maxtime / increments[i] > 12 && i < 7)
		i++;
	incr = increments[i];
	while (maxtime / incr > 12)
		incr *= 2;

	gc->leftx = 0; gc->rightx = maxtime;
	gc->topy = 0; gc->bottomy = 1.0;

	last_gc = *gc;

	QColor color;
	color = profile_color[TIME_GRID].at(0);
	for (i = incr; i < maxtime; i += incr) {
		QGraphicsLineItem *line = new QGraphicsLineItem(SCALE(gc, i, 0), SCALE(gc, i, 1));
		line->setPen(QPen(color));
		scene()->addItem(line);
	}

	/* now the text on the time markers */
	struct text_render_options tro = {DEPTH_TEXT_SIZE, TIME_TEXT, CENTER, TOP};
	if (maxtime < 600) {
		/* Be a bit more verbose with shorter dives */
		for (i = incr; i < maxtime; i += incr)
			plot_text(gc, &tro, i, 1, QString("%1:%2").arg(i/60).arg(i%60));
	} else {
		/* Only render the time on every second marker for normal dives */
		for (i = incr; i < maxtime; i += 2 * incr)
			plot_text(gc, &tro, i, 1, QString::number(i/60));
	}

	/* Depth markers: every 30 ft or 10 m*/
	gc->leftx = 0; gc->rightx = 1.0;
	gc->topy = 0; gc->bottomy = maxdepth;
	switch (prefs.units.length) {
	case units::METERS:
		marker = 10000;
		break;
	case units::FEET:
		marker = 9144;
		break;	/* 30 ft */
	}
	maxline = MAX(pi->maxdepth + marker, maxdepth * 2 / 3);

	color = profile_color[DEPTH_GRID].at(0);

	for (i = marker; i < maxline; i += marker) {
		QGraphicsLineItem *line = new QGraphicsLineItem(SCALE(gc, 0, i), SCALE(gc, 1, i));
		line->setPen(QPen(color));
		scene()->addItem(line);
	}

	gc->leftx = 0; gc->rightx = maxtime;
	color = profile_color[MEAN_DEPTH].at(0);

	/* Show mean depth */
	if (! gc->printer) {
		QGraphicsLineItem *line = new QGraphicsLineItem(SCALE(gc, 0, pi->meandepth), SCALE(gc, pi->entry[pi->nr - 1].sec, pi->meandepth));
		line->setPen(QPen(color));
		scene()->addItem(line);
	}

#if 0
	/*
	 * These are good for debugging text placement etc,
	 * but not for actual display..
	 */
	if (0) {
		plot_smoothed_profile(gc, pi);
		plot_minmax_profile(gc, pi);
	}
#endif

	/* Do the depth profile for the neat fill */
	gc->topy = 0; gc->bottomy = maxdepth;

	entry = pi->entry;

	QPolygonF p;
	QLinearGradient pat(0.0,0.0,0.0,scene()->height());
	QGraphicsPolygonItem *neatFill = NULL;

	p.append( QPointF(SCALE(gc, 0, 0) ));
	for (i = 0; i < pi->nr; i++, entry++)
		p.append( QPointF( SCALE(gc, entry->sec, entry->depth) ));

	/* Show any ceiling we may have encountered */
	for (i = pi->nr - 1; i >= 0; i--, entry--) {
		if (entry->ndl) {
			/* non-zero NDL implies this is a safety stop, no ceiling */
			p.append( QPointF( SCALE(gc, entry->sec, 0) ));
		} else if (entry->stopdepth < entry->depth) {
			p.append( QPointF( SCALE(gc, entry->sec, entry->stopdepth) ));
		} else {
			p.append( QPointF( SCALE(gc, entry->sec, entry->depth) ));
		}
	}
	pat.setColorAt(1, profile_color[DEPTH_BOTTOM].first());
	pat.setColorAt(0, profile_color[DEPTH_TOP].first());

	neatFill = new QGraphicsPolygonItem();
	neatFill->setPolygon(p);
	neatFill->setBrush(QBrush(pat));
	neatFill->setPen(QPen(QBrush(),0));
	scene()->addItem(neatFill);


	/* if the user wants the deco ceiling more visible, do that here (this
	 * basically draws over the background that we had allowed to shine
	 * through so far) */
	// TODO: port the prefs.profile_red_ceiling to QSettings

	//if (prefs.profile_red_ceiling) {
		p.clear();
		pat.setColorAt(0, profile_color[CEILING_SHALLOW].first());
		pat.setColorAt(1, profile_color[CEILING_DEEP].first());

		entry = pi->entry;
		p.append( QPointF(SCALE(gc, 0, 0) ));
		for (i = 0; i < pi->nr; i++, entry++) {
			if (entry->ndl == 0 && entry->stopdepth) {
				if (entry->ndl == 0 && entry->stopdepth < entry->depth) {
					p.append( QPointF( SCALE(gc, entry->sec, entry->stopdepth) ));
				} else {
					p.append( QPointF( SCALE(gc, entry->sec, entry->depth) ));
				}
			} else {
				p.append( QPointF( SCALE(gc, entry->sec, 0) ));
			}
		}

		neatFill = new QGraphicsPolygonItem();
		neatFill->setBrush(QBrush(pat));
		neatFill->setPolygon(p);
		neatFill->setPen(QPen(QBrush(),0));
		scene()->addItem(neatFill);
	//}

	/* finally, plot the calculated ceiling over all this */
	// TODO: Port the profile_calc_ceiling to QSettings
	// if (prefs.profile_calc_ceiling) {

		pat.setColorAt(0, profile_color[CALC_CEILING_SHALLOW].first());
		pat.setColorAt(1, profile_color[CALC_CEILING_DEEP].first());

		entry = pi->entry;
		p.clear();
		p.append( QPointF(SCALE(gc, 0, 0) ));
		for (i = 0; i < pi->nr; i++, entry++) {
			if (entry->ceiling)
				p.append( QPointF( SCALE(gc, entry->sec, entry->ceiling) ));
			else
				p.append( QPointF( SCALE(gc, entry->sec, 0) ));
		}
		p.append( QPointF( SCALE(gc, (entry-1)->sec, 0) ));
		neatFill = new QGraphicsPolygonItem();
		neatFill->setPolygon(p);
		neatFill->setPen(QPen(QBrush(),0));
		neatFill->setBrush(pat);
		scene()->addItem(neatFill);
	//}
	/* next show where we have been bad and crossed the dc's ceiling */
	pat.setColorAt(0, profile_color[CEILING_SHALLOW].first());
	pat.setColorAt(1, profile_color[CEILING_DEEP].first());

	entry = pi->entry;
	p.clear();
	p.append( QPointF(SCALE(gc, 0, 0) ));
	for (i = 0; i < pi->nr; i++, entry++)
		p.append( QPointF( SCALE(gc, entry->sec, entry->depth) ));

	for (i = pi->nr - 1; i >= 0; i--, entry--) {
		if (entry->ndl == 0 && entry->stopdepth > entry->depth) {
			p.append( QPointF( SCALE(gc, entry->sec, entry->stopdepth) ));
		} else {
			p.append( QPointF( SCALE(gc, entry->sec, entry->depth) ));
		}
	}

	neatFill = new QGraphicsPolygonItem();
	neatFill->setPolygon(p);
	neatFill->setPen(QPen(QBrush(),0));
	neatFill->setBrush(QBrush(pat));
	scene()->addItem(neatFill);

	/* Now do it again for the velocity colors */
	entry = pi->entry;
	for (i = 1; i < pi->nr; i++) {
		entry++;
		sec = entry->sec;
		/* we want to draw the segments in different colors
		 * representing the vertical velocity, so we need to
		 * chop this into short segments */
		depth = entry->depth;
		QGraphicsLineItem *colorLine = new QGraphicsLineItem( SCALE(gc, entry[-1].sec, entry[-1].depth), SCALE(gc, sec, depth));
		colorLine->setPen(QPen(QBrush(profile_color[ (color_indice_t) (VELOCITY_COLORS_START_IDX + entry->velocity)].first()), 2 ));
		scene()->addItem(colorLine);
	}
}

void ProfileGraphicsView::plot_text(struct graphics_context *gc, text_render_options_t *tro, double x, double y, const QString& text)
{
	QFontMetrics fm(font());

	double dx = tro->hpos * (fm.width(text));
	double dy = tro->vpos * (fm.height());

	QGraphicsSimpleTextItem *item = new QGraphicsSimpleTextItem(text);
	QPointF point( SCALE(gc, x, y) ); // This is neded because of the SCALE macro.

	item->setPos(point.x() + dx, point.y() +dy );
	item->setBrush( QBrush(profile_color[tro->color].first()));
	item->setPen( QPen(profile_color[BACKGROUND].first()));
	scene()->addItem(item);
	qDebug() << item->pos();
}


void ProfileGraphicsView::resizeEvent(QResizeEvent *event)
{
	// Fits the scene's rectangle on the view.
	// I can pass some parameters to this -
	// like Qt::IgnoreAspectRatio or Qt::KeepAspectRatio
	QRectF r = scene()->sceneRect();
	fitInView ( r.x() - 50, r.y() -50, r.width() + 100, r.height() + 100); // do a little bit of spacing;
}
