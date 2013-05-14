#include "profilegraphics.h"

#include <QGraphicsScene>
#include <QResizeEvent>
#include <QGraphicsLineItem>
#include <QPen>
#include <QBrush>
#include <QDebug>
#include <QLineF>
#include <QSettings>
#include <QIcon>
#include <QPropertyAnimation>
#include <QGraphicsSceneHoverEvent>
#include <QMouseEvent>

#include "../color.h"
#include "../display.h"
#include "../dive.h"
#include "../profile.h"

#include <libdivecomputer/parser.h>
#include <libdivecomputer/version.h>

#define SAC_COLORS_START_IDX SAC_1
#define SAC_COLORS 9
#define VELOCITY_COLORS_START_IDX VELO_STABLE
#define VELOCITY_COLORS 5

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

extern struct ev_select *ev_namelist;
extern int evn_allocated;
extern int evn_used;

ProfileGraphicsView::ProfileGraphicsView(QWidget* parent) : QGraphicsView(parent), toolTip(0) , dive(0)
{
	gc.printer = false;
	setScene(new QGraphicsScene());
	setBackgroundBrush(QColor("#F3F3E6"));
	scene()->installEventFilter(this);

	setRenderHint(QPainter::Antialiasing);
	setRenderHint(QPainter::HighQualityAntialiasing);
	setRenderHint(QPainter::SmoothPixmapTransform);

	defaultPen.setJoinStyle(Qt::RoundJoin);
	defaultPen.setCapStyle(Qt::RoundCap);
	defaultPen.setWidth(2);
	defaultPen.setCosmetic(true);

	setHorizontalScrollBarPolicy ( Qt::ScrollBarAlwaysOff );
	setVerticalScrollBarPolicy ( Qt::ScrollBarAlwaysOff );

	fill_profile_color();
}

void ProfileGraphicsView::wheelEvent(QWheelEvent* event)
{
	if (!toolTip)
		return;

    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

    // Scale the view / do the zoom
	QPoint toolTipPos = mapFromScene(toolTip->pos());
    double scaleFactor = 1.15;
    if(event->delta() > 0 && zoomLevel <= 10) {
        scale(scaleFactor, scaleFactor);
		zoomLevel++;
    } else if (zoomLevel >= 0) {
        // Zooming out
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
		zoomLevel--;
    }
    toolTip->setPos(mapToScene(toolTipPos).x(), mapToScene(toolTipPos).y());
}

void ProfileGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
	if (!toolTip)
		return;

	toolTip->refresh(&gc,  mapToScene(event->pos()));

	QPoint toolTipPos = mapFromScene(toolTip->pos());

	double dx = sceneRect().x();
	double dy = sceneRect().y();

	ensureVisible(event->pos().x() + dx, event->pos().y() + dy, 1, 1);

	toolTip->setPos(mapToScene(toolTipPos).x(), mapToScene(toolTipPos).y());

	if (zoomLevel < 0)
		QGraphicsView::mouseMoveEvent(event);
}

bool ProfileGraphicsView::eventFilter(QObject* obj, QEvent* event)
{
	// This will "Eat" the default tooltip behavior.
	if (event->type() == QEvent::GraphicsSceneHelp) {
		event->ignore();
		return true;
	}
	return QGraphicsView::eventFilter(obj, event);
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

void ProfileGraphicsView::showEvent(QShowEvent* event)
{
	if (dive)
		plot(dive);
}

void ProfileGraphicsView::plot(struct dive *d)
{

	scene()->clear();
	if (dive != d){
		resetTransform();
		zoomLevel = 0;
		dive = d;
		toolTip = 0;
	}

	if(!isVisible() || !dive){
		return;
	}

	scene()->setSceneRect(0,0, viewport()->width()-50, viewport()->height()-50);

	QSettings s;
	s.beginGroup("ProfileMap");
	QPointF toolTipPos = s.value("tooltip_position", QPointF(0,0)).toPointF();
	s.endGroup();

	toolTip = new ToolTipItem();
	toolTip->setPos(toolTipPos);

	scene()->addItem(toolTip);

	struct divecomputer *dc = &dive->dc;


	// Fix this for printing / screen later.
	// plot_set_scale(scale_mode_t);

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

	QRectF profile_grid_area = scene()->sceneRect();
	gc.maxx = (profile_grid_area.width() - 2 * profile_grid_area.x());
	gc.maxy = (profile_grid_area.height() - 2 * profile_grid_area.y());

	dc = select_dc(dc);

	/* This is per-dive-computer. Right now we just do the first one */
	gc.pi = *create_plot_info(dive, dc, &gc);

	/* Depth profile */
	plot_depth_profile();

	plot_events(dc);

	/* Temperature profile */
	plot_temperature_profile();

	/* Cylinder pressure plot */
	plot_cylinder_pressure(dive, dc);

	/* Text on top of all graphs.. */
	plot_temperature_text();

	plot_depth_text();

	plot_cylinder_pressure_text();

	plot_deco_text();

	/* Bounding box */
	QPen pen = defaultPen;
	pen.setColor(profile_color[TIME_GRID].at(0));
	QGraphicsRectItem *rect = new QGraphicsRectItem(profile_grid_area);
	rect->setPen(pen);
	scene()->addItem(rect);

	/* Put the dive computer name in the lower left corner */
	QString nick(get_dc_nickname(dc->model, dc->deviceid));
	if (nick.isEmpty())
		nick = QString(dc->model);

	if (nick.isEmpty())
		nick = tr("unknown divecomputer");

	text_render_options_t computer = {DC_TEXT_SIZE, TIME_TEXT, LEFT, MIDDLE};
	diveComputer = plot_text(&computer, QPointF(gc.leftx, gc.bottomy), nick);
	// The Time ruler should be right after the DiveComputer:
	timeMarkers->setPos(0, diveComputer->y());

	if (PP_GRAPHS_ENABLED) {
		plot_pp_gas_profile();
		plot_pp_text();
	}


	/* now shift the translation back by half the margin;
	 * this way we can draw the vertical scales on both sides */
	//cairo_translate(gc->cr, -drawing_area->x / 2.0, 0);

	//gc->maxx += drawing_area->x;
	//gc->leftx = -(drawing_area->x / drawing_area->width) / 2.0;
	//gc->rightx = 1.0 - gc->leftx;

	plot_depth_scale();

#if 0
	if (gc->printer) {
		free(pi->entry);
		last_pi_entry = pi->entry = NULL;
		pi->nr = 0;
	}
#endif

	QRectF r = scene()->itemsBoundingRect();
	scene()->setSceneRect(r.x() - 15, r.y() -15, r.width() + 30, r.height() + 30);
	if(zoomLevel == 0){
		fitInView(sceneRect());
	}
}

void ProfileGraphicsView::plot_depth_scale()
{
	int i, maxdepth, marker;
	static text_render_options_t tro = {DEPTH_TEXT_SIZE, SAMPLE_DEEP, RIGHT, MIDDLE};

	/* Depth markers: every 30 ft or 10 m*/
	maxdepth = get_maxdepth(&gc.pi);
	gc.topy = 0; gc.bottomy = maxdepth;

	switch (prefs.units.length) {
		case units::METERS: marker = 10000; break;
		case units::FEET: marker = 9144; break;	/* 30 ft */
	}

	QColor c(profile_color[DEPTH_GRID].first());

	/* don't write depth labels all the way to the bottom as
	 * there may be other graphs below the depth plot (like
	 * partial pressure graphs) where this would look out
	 * of place - so we only make sure that we print the next
	 * marker below the actual maxdepth of the dive */
	depthMarkers = new QGraphicsRectItem();
	for (i = marker; i <= gc.pi.maxdepth + marker; i += marker) {
		double d = get_depth_units(i, NULL, NULL);
		plot_text(&tro, QPointF(-0.002, i), QString::number(d), depthMarkers);
	}
	scene()->addItem(depthMarkers);
	depthMarkers->setPos(depthMarkers->pos().x() - 10, 0);
}

void ProfileGraphicsView::plot_pp_text()
{
	double pp, dpp, m;
	int hpos;
	static text_render_options_t tro = {PP_TEXT_SIZE, PP_LINES, LEFT, MIDDLE};

	setup_pp_limits(&gc);
	pp = floor(gc.pi.maxpp * 10.0) / 10.0 + 0.2;
	dpp = pp > 4 ? 1.0 : 0.5;
	hpos = gc.pi.entry[gc.pi.nr - 1].sec;
	QColor c = profile_color[PP_LINES].first();

	for (m = 0.0; m <= pp; m += dpp) {
		QGraphicsLineItem *item = new QGraphicsLineItem(SCALEGC(0, m), SCALEGC(hpos, m));
		QPen pen(defaultPen);
		pen.setColor(c);
		item->setPen(pen);
		scene()->addItem(item);
		plot_text(&tro, QPointF(hpos + 30, m), QString::number(m));
	}
}

void ProfileGraphicsView::plot_pp_gas_profile()
{
	int i;
	struct plot_data *entry;
	struct plot_info *pi = &gc.pi;

	setup_pp_limits(&gc);
	QColor c;
	QPointF from, to;
	//if (prefs.pp_graphs.pn2) {
		c = profile_color[PN2].first();
		entry = pi->entry;
		from = QPointF(SCALEGC(entry->sec, entry->pn2));
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->pn2 < prefs.pp_graphs.pn2_threshold){
				to = QPointF(SCALEGC(entry->sec, entry->pn2));
				QGraphicsLineItem *item = new QGraphicsLineItem(from.x(), from.y(), to.x(), to.y());
				QPen pen(defaultPen);
				pen.setColor(c);
				item->setPen(pen);
				scene()->addItem(item);
				from = to;
			}
			else{
				from = QPointF(SCALEGC(entry->sec, entry->pn2));
			}
		}

		c = profile_color[PN2_ALERT].first();
		entry = pi->entry;
		from = QPointF(SCALEGC(entry->sec, entry->pn2));
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->pn2 >= prefs.pp_graphs.pn2_threshold){
				to = QPointF(SCALEGC(entry->sec, entry->pn2));
				QGraphicsLineItem *item = new QGraphicsLineItem(from.x(), from.y(), to.x(), to.y());
				QPen pen(defaultPen);
				pen.setColor(c);
				item->setPen(pen);
				scene()->addItem(item);
				from = to;
			}
			else{
				from = QPointF(SCALEGC(entry->sec, entry->pn2));
			}
		}
	//}

	//if (prefs.pp_graphs.phe) {
		c = profile_color[PHE].first();
		entry = pi->entry;

		from = QPointF(SCALEGC(entry->sec, entry->phe));
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->phe < prefs.pp_graphs.phe_threshold){
				to = QPointF(SCALEGC(entry->sec, entry->phe));
				QGraphicsLineItem *item = new QGraphicsLineItem(from.x(), from.y(), to.x(), to.y());
				QPen pen(defaultPen);
				pen.setColor(c);
				item->setPen(pen);
				scene()->addItem(item);
				from = to;
			}
			else{
				from = QPointF(SCALEGC(entry->sec, entry->phe));
			}
		}

		c = profile_color[PHE_ALERT].first();
		entry = pi->entry;
		from = QPointF(SCALEGC(entry->sec, entry->phe));
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->phe >= prefs.pp_graphs.phe_threshold){
				to = QPointF(SCALEGC(entry->sec, entry->phe));
				QGraphicsLineItem *item = new QGraphicsLineItem(from.x(), from.y(), to.x(), to.y());
				QPen pen(defaultPen);
				pen.setColor(c);
				item->setPen(pen);
				scene()->addItem(item);
				from = to;
			}
			else{
				from = QPointF(SCALEGC(entry->sec, entry->phe));
			}
		}
	//}
	//if (prefs.pp_graphs.po2) {
		c = profile_color[PO2].first();
		entry = pi->entry;
		from = QPointF(SCALEGC(entry->sec, entry->po2));
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->po2 < prefs.pp_graphs.po2_threshold){
				to = QPointF(SCALEGC(entry->sec, entry->po2));
				QGraphicsLineItem *item = new QGraphicsLineItem(from.x(), from.y(), to.x(), to.y());
				QPen pen(defaultPen);
				pen.setColor(c);
				item->setPen(pen);
				scene()->addItem(item);
				from = to;
			}
			else{
				from = QPointF(SCALEGC(entry->sec, entry->po2));
			}
		}

		c = profile_color[PO2_ALERT].first();
		entry = pi->entry;
		from = QPointF(SCALEGC(entry->sec, entry->po2));
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->po2 >= prefs.pp_graphs.po2_threshold){
				to = QPointF(SCALEGC(entry->sec, entry->po2));
				QGraphicsLineItem *item = new QGraphicsLineItem(from.x(), from.y(), to.x(), to.y());
				item->setPen(QPen(c));
				scene()->addItem(item);
				from = to;
			}
			else{
				from = QPointF(SCALEGC(entry->sec, entry->po2));
			}
		}
	//}
}

void ProfileGraphicsView::plot_deco_text()
{
	if (prefs.profile_calc_ceiling) {
		float x = gc.leftx + (gc.rightx - gc.leftx) / 2;
		float y = gc.topy = 1.0;
		static text_render_options_t tro = {PRESSURE_TEXT_SIZE, PRESSURE_TEXT, CENTER, -0.2};
		gc.bottomy = 0.0;
		plot_text(&tro, QPointF(x, y), QString("GF %1/%2").arg(prefs.gflow * 100).arg(prefs.gfhigh * 100));
	}
}

void ProfileGraphicsView::plot_cylinder_pressure_text()
{
	int i;
	int mbar, cyl;
	int seen_cyl[MAX_CYLINDERS] = { FALSE, };
	int last_pressure[MAX_CYLINDERS] = { 0, };
	int last_time[MAX_CYLINDERS] = { 0, };
	struct plot_data *entry;
	struct plot_info *pi = &gc.pi;

	if (!get_cylinder_pressure_range(&gc))
		return;

	cyl = -1;
	for (i = 0; i < pi->nr; i++) {
		entry = pi->entry + i;
		mbar = GET_PRESSURE(entry);

		if (!mbar)
			continue;
		if (cyl != entry->cylinderindex) {
			cyl = entry->cylinderindex;
			if (!seen_cyl[cyl]) {
				plot_pressure_value(mbar, entry->sec, LEFT, BOTTOM);
				seen_cyl[cyl] = TRUE;
			}
		}
		last_pressure[cyl] = mbar;
		last_time[cyl] = entry->sec;
	}

	for (cyl = 0; cyl < MAX_CYLINDERS; cyl++) {
		if (last_time[cyl]) {
			plot_pressure_value(last_pressure[cyl], last_time[cyl], CENTER, TOP);
		}
	}
}

void ProfileGraphicsView::plot_pressure_value(int mbar, int sec, double xalign, double yalign)
{
	int pressure;
	const char *unit;

	pressure = get_pressure_units(mbar, &unit);
	static text_render_options_t tro = {PRESSURE_TEXT_SIZE, PRESSURE_TEXT, xalign, yalign};
	plot_text(&tro, QPointF(sec, mbar), QString("%1 %2").arg(pressure).arg(unit));
}

void ProfileGraphicsView::plot_depth_text()
{
	int maxtime, maxdepth;

	/* Get plot scaling limits */
	maxtime = get_maxtime(&gc.pi);
	maxdepth = get_maxdepth(&gc.pi);

	gc.leftx = 0; gc.rightx = maxtime;
	gc.topy = 0; gc.bottomy = maxdepth;

	plot_text_samples();
}

void ProfileGraphicsView::plot_text_samples()
{
	static text_render_options_t deep = {14, SAMPLE_DEEP, CENTER, TOP};
	static text_render_options_t shallow = {14, SAMPLE_SHALLOW, CENTER, BOTTOM};
	int i;
	int last = -1;

	struct plot_info* pi = &gc.pi;

	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry + i;

		if (entry->depth < 2000)
			continue;

		if ((entry == entry->max[2]) && entry->depth / 100 != last) {
			plot_depth_sample(entry, &deep);
			last = entry->depth / 100;
		}

		if ((entry == entry->min[2]) && entry->depth / 100 != last) {
			plot_depth_sample(entry, &shallow);
			last = entry->depth / 100;
		}

		if (entry->depth != last)
			last = -1;
	}
}

void ProfileGraphicsView::plot_depth_sample(struct plot_data *entry,text_render_options_t *tro)
{
	int sec = entry->sec, decimals;
	double d;

	d = get_depth_units(entry->depth, &decimals, NULL);

	plot_text(tro, QPointF(sec, entry->depth), QString("%1").arg(d, 0, 'f', 1));
}


void ProfileGraphicsView::plot_temperature_text()
{
	int i;
	int last = -300, sec = 0;
	int last_temperature = 0, last_printed_temp = 0;
	plot_info *pi = &gc.pi;

	if (!setup_temperature_limits(&gc))
		return;

	for (i = 0; i < pi->nr; i++) {
		struct plot_data *entry = pi->entry+i;
		int mkelvin = entry->temperature;
		sec = entry->sec;

		if (!mkelvin)
			continue;
		last_temperature = mkelvin;
		/* don't print a temperature
		 * if it's been less than 5min and less than a 2K change OR
		 * if it's been less than 2min OR if the change from the
		 * last print is less than .4K (and therefore less than 1F) */
		if (((sec < last + 300) && (abs(mkelvin - last_printed_temp) < 2000)) ||
			(sec < last + 120) ||
			(abs(mkelvin - last_printed_temp) < 400))
			continue;
		last = sec;
		if (mkelvin > 200000)
			plot_single_temp_text(sec,mkelvin);
		last_printed_temp = mkelvin;
	}
	/* it would be nice to print the end temperature, if it's
	 * different or if the last temperature print has been more
	 * than a quarter of the dive back */
	if (last_temperature > 200000 &&
	    ((abs(last_temperature - last_printed_temp) > 500) || ((double)last / (double)sec < 0.75)))
		plot_single_temp_text(sec, last_temperature);
}

void ProfileGraphicsView::plot_single_temp_text(int sec, int mkelvin)
{
	double deg;
	const char *unit;
	static text_render_options_t tro = {TEMP_TEXT_SIZE, TEMP_TEXT, LEFT, TOP};
	deg = get_temp_units(mkelvin, &unit);
	plot_text(&tro, QPointF(sec, mkelvin), QString("%1%2").arg(deg, 0, 'f', 1).arg(unit)); //"%.2g%s"
}

void ProfileGraphicsView::plot_cylinder_pressure(struct dive *dive, struct divecomputer *dc)
{
	int i;
	int last = -1, last_index = -1;
	int lift_pen = FALSE;
	int first_plot = TRUE;
	int sac = 0;
	struct plot_data *last_entry = NULL;

	if (!get_cylinder_pressure_range(&gc))
		return;

	QPointF from, to;
	for (i = 0; i < gc.pi.nr; i++) {
		int mbar;
		struct plot_data *entry = gc.pi.entry + i;

		mbar = GET_PRESSURE(entry);
		if (entry->cylinderindex != last_index) {
			lift_pen = TRUE;
			last_entry = NULL;
		}
		if (!mbar) {
			lift_pen = TRUE;
			continue;
		}
		if (!last_entry) {
			last = i;
			last_entry = entry;
			sac = get_local_sac(entry, gc.pi.entry + i + 1, dive);
		} else {
			int j;
			sac = 0;
			for (j = last; j < i; j++)
				sac += get_local_sac(gc.pi.entry + j, gc.pi.entry + j + 1, dive);
			sac /= (i - last);
			if (entry->sec - last_entry->sec >= SAC_WINDOW) {
				last++;
				last_entry = gc.pi.entry + last;
			}
		}

		QColor c = get_sac_color(sac, dive->sac);

		if (lift_pen) {
			if (!first_plot && entry->cylinderindex == last_index) {
				/* if we have a previous event from the same tank,
				 * draw at least a short line */
				int prev_pr;
				prev_pr = GET_PRESSURE(entry - 1);

				QGraphicsLineItem *item = new QGraphicsLineItem(SCALEGC((entry-1)->sec, prev_pr), SCALEGC(entry->sec, mbar));
				QPen pen(defaultPen);
				pen.setColor(c);
				item->setPen(pen);
				scene()->addItem(item);
			} else {
				first_plot = FALSE;
				from = QPointF(SCALEGC(entry->sec, mbar));
			}
			lift_pen = FALSE;
		} else {
			to = QPointF(SCALEGC(entry->sec, mbar));
			QGraphicsLineItem *item = new QGraphicsLineItem(from.x(), from.y(), to.x(), to.y());
			QPen pen(defaultPen);
			pen.setColor(c);
			item->setPen(pen);
			scene()->addItem(item);
		}


		from = QPointF(SCALEGC(entry->sec, mbar));
		last_index = entry->cylinderindex;
	}
}


/* set the color for the pressure plot according to temporary sac rate
 * as compared to avg_sac; the calculation simply maps the delta between
 * sac and avg_sac to indexes 0 .. (SAC_COLORS - 1) with everything
 * more than 6000 ml/min below avg_sac mapped to 0 */
QColor ProfileGraphicsView::get_sac_color(int sac, int avg_sac)
{
	int sac_index = 0;
	int delta = sac - avg_sac + 7000;

	if (!gc.printer) {
		sac_index = delta / 2000;
		if (sac_index < 0)
			sac_index = 0;
		if (sac_index > SAC_COLORS - 1)
			sac_index = SAC_COLORS - 1;
		return profile_color[ (color_indice_t) (SAC_COLORS_START_IDX + sac_index)].first();
	}
	return profile_color[SAC_DEFAULT].first();
}

void ProfileGraphicsView::plot_events(struct divecomputer *dc)
{
	struct event *event = dc->events;

// 	if (gc->printer){
// 		return;
// 	}

	while (event) {
		plot_one_event(event);
		event = event->next;
	}
}

void ProfileGraphicsView::plot_one_event(struct event *ev)
{
	int i, depth = 0;
	struct plot_info *pi = &gc.pi;

	/* is plotting of this event disabled? */
	if (ev->name) {
		for (i = 0; i < evn_used; i++) {
			if (! strcmp(ev->name, ev_namelist[i].ev_name)) {
				if (ev_namelist[i].plot_ev)
					break;
				else
					return;
			}
		}
	}

	if (ev->time.seconds < 30 && !strcmp(ev->name, "gaschange"))
		/* a gas change in the first 30 seconds is the way of some dive computers
		 * to tell us the gas that is used; let's not plot a marker for that */
		return;

	for (i = 0; i < pi->nr; i++) {
		struct plot_data *data = pi->entry + i;
		if (ev->time.seconds < data->sec)
			break;
		depth = data->depth;
	}

	/* draw a little triangular marker and attach tooltip */

	int x = SCALEXGC(ev->time.seconds);
	int y = SCALEYGC(depth);

	EventItem *item = new EventItem();
	item->setPos(x, y);
	scene()->addItem(item);

	/* we display the event on screen - so translate */
	QString name = tr(ev->name);
	if (ev->value) {
		if (ev->name && name == "gaschange") {
			unsigned int he = ev->value >> 16;
			unsigned int o2 = ev->value & 0xffff;

			name += ": ";
			name += (he) ? QString("%1/%2").arg(o2, he)
				  : (o2 == 21) ? name += tr("air")
				  : QString("%1% %2").arg(o2).arg("O" UTF8_SUBSCRIPT_2);

		} else if (ev->name && !strcmp(ev->name, "SP change")) {
			name += QString(":%1").arg((double) ev->value / 1000);
		} else {
			name += QString(":%1").arg(ev->value);
		}
	} else if (ev->name && name == "SP change") {
		name += tr("Bailing out to OC");
	} else {
		name += ev->flags == SAMPLE_FLAGS_BEGIN ? tr("Starts with space!"," begin") :
				ev->flags == SAMPLE_FLAGS_END ? tr("Starts with space!", " end") : "";
	}

	//item->setToolTipController(toolTip);
	//item->addToolTip(name);
	item->setToolTip(name);
}

void ProfileGraphicsView::plot_depth_profile()
{
	int i, incr;
	int sec, depth;
	struct plot_data *entry;
	int maxtime, maxdepth, marker, maxline;
	int increments[8] = { 10, 20, 30, 60, 5*60, 10*60, 15*60, 30*60 };

	/* Get plot scaling limits */
	maxtime = get_maxtime(&gc.pi);
	maxdepth = get_maxdepth(&gc.pi);

	gc.maxtime = maxtime;

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

	gc.leftx = 0; gc.rightx = maxtime;
	gc.topy = 0; gc.bottomy = 1.0;

	last_gc = gc;

	QColor c = profile_color[TIME_GRID].at(0);
	for (i = incr; i < maxtime; i += incr) {
		QGraphicsLineItem *item = new QGraphicsLineItem(SCALEGC(i, 0), SCALEGC(i, 1));
		QPen pen(defaultPen);
		pen.setColor(c);
		item->setPen(pen);
		scene()->addItem(item);
	}

	timeMarkers = new QGraphicsRectItem();
	/* now the text on the time markers */
	struct text_render_options tro = {DEPTH_TEXT_SIZE, TIME_TEXT, CENTER, TOP};
	if (maxtime < 600) {
		/* Be a bit more verbose with shorter dives */
		for (i = incr; i < maxtime; i += incr)
			plot_text(&tro, QPointF(i, 0), QString("%1:%2").arg(i/60).arg(i%60), timeMarkers);
	} else {
		/* Only render the time on every second marker for normal dives */
		for (i = incr; i < maxtime; i += 2 * incr)
			plot_text(&tro, QPointF(i, 0), QString("%1").arg(QString::number(i/60)), timeMarkers);
	}
	timeMarkers->setPos(0,0);
	scene()->addItem(timeMarkers);

	/* Depth markers: every 30 ft or 10 m*/
	gc.leftx = 0; gc.rightx = 1.0;
	gc.topy = 0; gc.bottomy = maxdepth;
	switch (prefs.units.length) {
	case units::METERS:
		marker = 10000;
		break;
	case units::FEET:
		marker = 9144;
		break;	/* 30 ft */
	}
	maxline = MAX(gc.pi.maxdepth + marker, maxdepth * 2 / 3);

	c = profile_color[DEPTH_GRID].at(0);

	for (i = marker; i < maxline; i += marker) {
		QGraphicsLineItem *item = new QGraphicsLineItem(SCALEGC(0, i), SCALEGC(1, i));
		QPen pen(defaultPen);
		pen.setColor(c);
		item->setPen(pen);
		scene()->addItem(item);
	}

	gc.leftx = 0; gc.rightx = maxtime;
	c = profile_color[MEAN_DEPTH].at(0);

	/* Show mean depth */
	if (! gc.printer) {
		QGraphicsLineItem *item = new QGraphicsLineItem(SCALEGC(0, gc.pi.meandepth),
								SCALEGC(gc.pi.entry[gc.pi.nr - 1].sec, gc.pi.meandepth));
		QPen pen(defaultPen);
		pen.setColor(c);
		item->setPen(pen);
		scene()->addItem(item);
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
	gc.topy = 0; gc.bottomy = maxdepth;

	entry = gc.pi.entry;

	QPolygonF p;
	QLinearGradient pat(0.0,0.0,0.0,scene()->height());
	QGraphicsPolygonItem *neatFill = NULL;

	p.append(QPointF(SCALEGC(0, 0)));
	for (i = 0; i < gc.pi.nr; i++, entry++)
		p.append(QPointF(SCALEGC(entry->sec, entry->depth)));

	/* Show any ceiling we may have encountered */
	for (i = gc.pi.nr - 1; i >= 0; i--, entry--) {
		if (entry->ndl) {
			/* non-zero NDL implies this is a safety stop, no ceiling */
			p.append(QPointF(SCALEGC(entry->sec, 0)));
		} else if (entry->stopdepth < entry->depth) {
			p.append(QPointF(SCALEGC(entry->sec, entry->stopdepth)));
		} else {
			p.append(QPointF(SCALEGC(entry->sec, entry->depth)));
		}
	}
	pat.setColorAt(1, profile_color[DEPTH_BOTTOM].first());
	pat.setColorAt(0, profile_color[DEPTH_TOP].first());

	neatFill = new QGraphicsPolygonItem();
	neatFill->setPolygon(p);
	neatFill->setBrush(QBrush(pat));
	neatFill->setPen(QPen(QBrush(Qt::transparent),0));
	scene()->addItem(neatFill);


	/* if the user wants the deco ceiling more visible, do that here (this
	 * basically draws over the background that we had allowed to shine
	 * through so far) */
	// TODO: port the prefs.profile_red_ceiling to QSettings

	//if (prefs.profile_red_ceiling) {
		p.clear();
		pat.setColorAt(0, profile_color[CEILING_SHALLOW].first());
		pat.setColorAt(1, profile_color[CEILING_DEEP].first());

		entry = gc.pi.entry;
		p.append(QPointF(SCALEGC(0, 0)));
		for (i = 0; i < gc.pi.nr; i++, entry++) {
			if (entry->ndl == 0 && entry->stopdepth) {
				if (entry->ndl == 0 && entry->stopdepth < entry->depth) {
					p.append(QPointF(SCALEGC(entry->sec, entry->stopdepth)));
				} else {
					p.append(QPointF(SCALEGC(entry->sec, entry->depth)));
				}
			} else {
				p.append(QPointF(SCALEGC(entry->sec, 0)));
			}
		}

		neatFill = new QGraphicsPolygonItem();
		neatFill->setBrush(QBrush(pat));
		neatFill->setPolygon(p);
		neatFill->setPen(QPen(QBrush(Qt::NoBrush),0));
		scene()->addItem(neatFill);
	//}

	/* finally, plot the calculated ceiling over all this */
	// TODO: Port the profile_calc_ceiling to QSettings
	// if (prefs.profile_calc_ceiling) {

		pat.setColorAt(0, profile_color[CALC_CEILING_SHALLOW].first());
		pat.setColorAt(1, profile_color[CALC_CEILING_DEEP].first());

		entry = gc.pi.entry;
		p.clear();
		p.append(QPointF(SCALEGC(0, 0)));
		for (i = 0; i < gc.pi.nr; i++, entry++) {
			if (entry->ceiling)
				p.append(QPointF(SCALEGC(entry->sec, entry->ceiling)));
			else
				p.append(QPointF(SCALEGC(entry->sec, 0)));
		}
		p.append(QPointF(SCALEGC((entry-1)->sec, 0)));
		neatFill = new QGraphicsPolygonItem();
		neatFill->setPolygon(p);
		neatFill->setPen(QPen(QBrush(Qt::NoBrush),0));
		neatFill->setBrush(pat);
		scene()->addItem(neatFill);
	//}
	/* next show where we have been bad and crossed the dc's ceiling */
	pat.setColorAt(0, profile_color[CEILING_SHALLOW].first());
	pat.setColorAt(1, profile_color[CEILING_DEEP].first());

	entry = gc.pi.entry;
	p.clear();
	p.append(QPointF(SCALEGC(0, 0)));
	for (i = 0; i < gc.pi.nr; i++, entry++)
		p.append(QPointF(SCALEGC(entry->sec, entry->depth)));

	for (i = gc.pi.nr - 1; i >= 0; i--, entry--) {
		if (entry->ndl == 0 && entry->stopdepth > entry->depth) {
			p.append(QPointF(SCALEGC(entry->sec, entry->stopdepth)));
		} else {
			p.append(QPointF(SCALEGC(entry->sec, entry->depth)));
		}
	}

	neatFill = new QGraphicsPolygonItem();
	neatFill->setPolygon(p);
	neatFill->setPen(QPen(QBrush(Qt::NoBrush),0));
	neatFill->setBrush(QBrush(pat));
	scene()->addItem(neatFill);

	/* Now do it again for the velocity colors */
	entry = gc.pi.entry;
	for (i = 1; i < gc.pi.nr; i++) {
		entry++;
		sec = entry->sec;
		/* we want to draw the segments in different colors
		 * representing the vertical velocity, so we need to
		 * chop this into short segments */
		depth = entry->depth;
		QGraphicsLineItem *item = new QGraphicsLineItem(SCALEGC(entry[-1].sec, entry[-1].depth), SCALEGC(sec, depth));
		QPen pen(defaultPen);
		pen.setColor(profile_color[ (color_indice_t) (VELOCITY_COLORS_START_IDX + entry->velocity)].first());
		item->setPen(pen);
		scene()->addItem(item);
	}
}

QGraphicsSimpleTextItem *ProfileGraphicsView::plot_text(text_render_options_t *tro,const QPointF& pos, const QString& text, QGraphicsItem *parent)
{
	QFontMetrics fm(font());

	double dx = tro->hpos * (fm.width(text));
	double dy = tro->vpos * (fm.height());

	QGraphicsSimpleTextItem *item = new QGraphicsSimpleTextItem(text, parent);
	QPointF point(SCALEGC(pos.x(), pos.y())); // This is neded because of the SCALE macro.

	item->setPos(point.x() + dx, point.y() + dy);
	item->setBrush(QBrush(profile_color[tro->color].first()));
	item->setFlag(QGraphicsItem::ItemIgnoresTransformations);

	if(!parent)
		scene()->addItem(item);
	return item;
}

void ProfileGraphicsView::resizeEvent(QResizeEvent *event)
{
	plot(dive);
}

void ProfileGraphicsView::plot_temperature_profile()
{
	int last = 0;

	if (!setup_temperature_limits(&gc))
		return;

	QPointF from;
	QPointF to;
	QColor color = profile_color[TEMP_PLOT].first();

	for (int i = 0; i < gc.pi.nr; i++) {
		struct plot_data *entry = gc.pi.entry + i;
		int mkelvin = entry->temperature;
		int sec = entry->sec;
		if (!mkelvin) {
			if (!last)
				continue;
			mkelvin = last;
		}
		if (last) {
			to = QPointF(SCALEGC(sec, mkelvin));
			QGraphicsLineItem *item = new QGraphicsLineItem(from.x(), from.y(), to.x(), to.y());
			QPen pen(defaultPen);
			pen.setColor(color);
			item->setPen(pen);
			scene()->addItem(item);
			from = to;
		}
		else{
			from = QPointF(SCALEGC(sec, mkelvin));
		}
		last = mkelvin;
	}
}

void ToolTipItem::addToolTip(const QString& toolTip, const QIcon& icon)
{
	QGraphicsPixmapItem *iconItem = 0;
	double yValue = title->boundingRect().height() + SPACING;
	Q_FOREACH(ToolTip t, toolTips) {
		yValue += t.second->boundingRect().height();
	}
	if (!icon.isNull()) {
		iconItem = new QGraphicsPixmapItem(icon.pixmap(ICON_SMALL,ICON_SMALL), this);
		iconItem->setPos(SPACING, yValue);
	}

	QGraphicsSimpleTextItem *textItem = new QGraphicsSimpleTextItem(toolTip, this);
	textItem->setPos(SPACING + ICON_SMALL + SPACING, yValue);
	textItem->setBrush(QBrush(Qt::white));
	textItem->setFlag(ItemIgnoresTransformations);
	toolTips[toolTip] = qMakePair(iconItem, textItem);
	expand();
}

void ToolTipItem::removeToolTip(const QString& toolTip)
{
	ToolTip toBeRemoved = toolTips[toolTip];
	delete toBeRemoved.first;
	delete toBeRemoved.second;
	toolTips.remove(toolTip);

	int toolTipIndex = 0;

	// We removed a toolTip, let's move the others to the correct location
	Q_FOREACH(ToolTip t, toolTips) {
		double yValue = title->boundingRect().height() + SPACING + toolTipIndex * ICON_SMALL + SPACING;

		// Icons can be null.
		if (t.first)
			t.first->setPos(SPACING, yValue);

		t.second->setPos(SPACING + ICON_SMALL + SPACING, yValue);
		toolTipIndex++;
	}

	expand();
}

void ToolTipItem::refresh(struct graphics_context *gc, QPointF pos)
{
	clear();
	int time = (pos.x() * gc->maxtime) / scene()->sceneRect().width();
	char buffer[500];
	get_plot_details(gc, time, buffer, 500);
	addToolTip(QString(buffer));

	QList<QGraphicsItem*> items = scene()->items(pos, Qt::IntersectsItemShape, Qt::DescendingOrder, transform());
	Q_FOREACH(QGraphicsItem *item, items) {
		if (!item->toolTip().isEmpty())
			addToolTip(item->toolTip());
	}

}

void ToolTipItem::clear()
{
	Q_FOREACH(ToolTip t, toolTips) {
		delete t.first;
		delete t.second;
	}
	toolTips.clear();
	expand();
}

void ToolTipItem::setRect(const QRectF& r)
{

	// qDeleteAll(childItems());
	delete background;

	rectangle = r;
	setBrush(QBrush(Qt::white));
	setPen(QPen(Qt::black, 0.5));

	// Creates a 2pixels border
	QPainterPath border;
	border.addRoundedRect(-4, -4,  rectangle.width() + 8, rectangle.height() + 10, 3, 3);
	border.addRoundedRect(-1, -1,  rectangle.width() + 3, rectangle.height() + 4, 3, 3);
	setPath(border);

	QPainterPath bg;
	bg.addRoundedRect(-1, -1, rectangle.width() + 3, rectangle.height() + 4, 3, 3);

	QColor c = QColor(Qt::black);
	c.setAlpha(155);

	QGraphicsPathItem *b = new QGraphicsPathItem(bg, this);
	b->setFlag(ItemStacksBehindParent);
	b->setFlags(ItemIgnoresTransformations);
	b->setBrush(c);
	b->setPen(QPen(QBrush(Qt::transparent), 0));
	b->setZValue(-10);
	background = b;

	updateTitlePosition();
}


void ToolTipItem::collapse()
{
	QPropertyAnimation *animation = new QPropertyAnimation(this, "rect");
	animation->setDuration(100);
	animation->setStartValue(boundingRect());
	animation->setEndValue(QRect(0, 0, ICON_SMALL, ICON_SMALL));
	animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void ToolTipItem::expand()
{

	if (!title){
		return;
	}

	QRectF nextRectangle;
	double width = 0, height = title->boundingRect().height() + SPACING;
	Q_FOREACH(ToolTip t, toolTips) {
		if (t.second->boundingRect().width() > width)
			width = t.second->boundingRect().width();
		height += t.second->boundingRect().height();
	}
	/*       Left padding, Icon Size,   space, right padding */
	width += SPACING       + ICON_SMALL + SPACING + SPACING;

	if (width < title->boundingRect().width() + SPACING*2)
		width = title->boundingRect().width() + SPACING*2;

	if(height < ICON_SMALL)
		height = ICON_SMALL;

	nextRectangle.setWidth(width);
	nextRectangle.setHeight(height);

	QPropertyAnimation *animation = new QPropertyAnimation(this, "rect");
	animation->setDuration(100);
	animation->setStartValue(rectangle);
	animation->setEndValue(nextRectangle);
	animation->start(QAbstractAnimation::DeleteWhenStopped);

}

ToolTipItem::ToolTipItem(QGraphicsItem* parent): QGraphicsPathItem(parent), background(0)
{
	title = new QGraphicsSimpleTextItem(tr("Information"), this);
	separator = new QGraphicsLineItem(this);

	setFlag(ItemIgnoresTransformations);
	setFlag(ItemIsMovable);

	updateTitlePosition();
	setZValue(99);
}

void ToolTipItem::updateTitlePosition()
{
	if (rectangle.width() < title->boundingRect().width() + SPACING*4) {
		QRectF newRect = rectangle;
		newRect.setWidth(title->boundingRect().width() + SPACING*4);
		newRect.setHeight(newRect.height() ? newRect.height() : ICON_SMALL);
		setRect(newRect);
	}

	title->setPos(boundingRect().width()/2  - title->boundingRect().width()/2 -1, 0);
	title->setFlag(ItemIgnoresTransformations);
	title->setPen(QPen(Qt::white, 1));
	title->setBrush(Qt::white);

	if (toolTips.size() > 0) {
		double x1 = 0;
		double y1 = title->pos().y() + SPACING/2 + title->boundingRect().height();
		double x2 = boundingRect().width() - 10;
		double y2 = y1;

		separator->setLine(x1, y1, x2, y2);
		separator->setFlag(ItemIgnoresTransformations);
		separator->setPen(QPen(Qt::white));
		separator->show();
	} else {
		separator->hide();
	}
}

EventItem::EventItem(QGraphicsItem* parent): QGraphicsPolygonItem(parent)
{
	setFlag(ItemIgnoresTransformations);
	setFlag(ItemIsFocusable);
	setAcceptHoverEvents(true);

	QPolygonF poly;
	poly.push_back(QPointF(-8, 16));
	poly.push_back(QPointF(8, 16));
	poly.push_back(QPointF(0, 0));
	poly.push_back(QPointF(-8, 16));

	QPen defaultPen ;
	defaultPen.setJoinStyle(Qt::RoundJoin);
	defaultPen.setCapStyle(Qt::RoundCap);
	defaultPen.setWidth(2);
	defaultPen.setCosmetic(true);

	QPen pen = defaultPen;
	pen.setBrush(QBrush(profile_color[ALERT_BG].first()));

	setPolygon(poly);
	setBrush(QBrush(profile_color[ALERT_BG].first()));
	setPen(pen);

	QGraphicsLineItem *line = new QGraphicsLineItem(0,5,0,10, this);
	line->setPen(QPen(Qt::black, 2));

	QGraphicsEllipseItem *ball = new QGraphicsEllipseItem(-1, 12, 2,2, this);
	ball->setBrush(QBrush(Qt::black));

}
