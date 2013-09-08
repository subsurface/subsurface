#include "profilegraphics.h"
#include "mainwindow.h"
#include "divelistview.h"

#include <QGraphicsScene>
#include <QResizeEvent>
#include <QGraphicsLineItem>
#include <QScrollBar>
#include <QPen>
#include <QBrush>
#include <QDebug>
#include <QLineF>
#include <QSettings>
#include <QIcon>
#include <QPropertyAnimation>
#include <QGraphicsSceneHoverEvent>
#include <QMouseEvent>
#include <qtextdocument.h>

#include "../color.h"
#include "../display.h"
#include "../dive.h"
#include "../profile.h"
#include "../device.h"
#include "../helpers.h"

#include <libdivecomputer/parser.h>
#include <libdivecomputer/version.h>

static struct graphics_context last_gc;

#if PRINT_IMPLEMENTED
static double plot_scale = SCALE_SCREEN;
#endif

struct text_render_options{
	double size;
	color_indice_t color;
	double hpos, vpos;
};

extern struct ev_select *ev_namelist;
extern int evn_allocated;
extern int evn_used;

ProfileGraphicsView::ProfileGraphicsView(QWidget* parent) : QGraphicsView(parent), toolTip(0) , dive(0), diveDC(0)
{
	printMode = false;
	isGrayscale = false;
	gc.printer = false;
	fill_profile_color();
	setScene(new QGraphicsScene());

	scene()->installEventFilter(this);

	setRenderHint(QPainter::Antialiasing);
	setRenderHint(QPainter::HighQualityAntialiasing);
	setRenderHint(QPainter::SmoothPixmapTransform);

	defaultPen.setJoinStyle(Qt::RoundJoin);
	defaultPen.setCapStyle(Qt::RoundCap);
	defaultPen.setWidth(2);
	defaultPen.setCosmetic(true);

	setHorizontalScrollBarPolicy (Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy (Qt::ScrollBarAlwaysOff);


}

/* since we cannot use translate() directly on the scene we hack on
 * the scroll bars (hidden) functionality */
void ProfileGraphicsView::scrollViewTo(const QPoint pos)
{
	if (!zoomLevel)
		return;
	QScrollBar *vs = verticalScrollBar();
	QScrollBar *hs = horizontalScrollBar();
	const qreal yRat = pos.y() / sceneRect().height();
	const qreal xRat = pos.x() / sceneRect().width();
	const int vMax = vs->maximum();
	const int hMax = hs->maximum();
	const int vMin = vs->minimum();
	const int hMin = hs->minimum();
	/* QScrollBar receives crazy negative values for minimum */
	vs->setValue(yRat * (vMax - vMin) + vMin * 0.9);
	hs->setValue(xRat * (hMax - hMin) + hMin * 0.9);
}

void ProfileGraphicsView::wheelEvent(QWheelEvent* event)
{
	if (!toolTip)
		return;

	// doesn't seem to work for Qt 4.8.1
	// setTransformationAnchor(QGraphicsView::AnchorUnderMouse);

	// Scale the view / do the zoom
	QPoint toolTipPos = mapFromScene(toolTip->pos());

	double scaleFactor = 1.15;
	if (event->delta() > 0 && zoomLevel < 20) {
		scale(scaleFactor, scaleFactor);
		zoomLevel++;
	} else if (event->delta() < 0 && zoomLevel > 0) {
		// Zooming out
		scale(1.0 / scaleFactor, 1.0 / scaleFactor);
		zoomLevel--;
	}
	scrollViewTo(event->pos());
	toolTip->setPos(mapToScene(toolTipPos).x(), mapToScene(toolTipPos).y());
}

void ProfileGraphicsView::mouseMoveEvent(QMouseEvent* event)
{
	if (!toolTip)
		return;

	toolTip->refresh(&gc,  mapToScene(event->pos()));

	QPoint toolTipPos = mapFromScene(toolTip->pos());

	scrollViewTo(event->pos());

	if (zoomLevel == 0)
		QGraphicsView::mouseMoveEvent(event);
	else
		toolTip->setPos(mapToScene(toolTipPos).x(), mapToScene(toolTipPos).y());
}

bool ProfileGraphicsView::eventFilter(QObject* obj, QEvent* event)
{
	if (event->type() == QEvent::Leave) {
		if (toolTip && toolTip->isExpanded())
			toolTip->collapse();
		return true;
	}

	// This will "Eat" the default tooltip behavior.
	if (event->type() == QEvent::GraphicsSceneHelp) {
		event->ignore();
		return true;
	}
	return QGraphicsView::eventFilter(obj, event);
}

#if PRINT_IMPLEMENTED
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
#endif

void ProfileGraphicsView::showEvent(QShowEvent* event)
{
	// Program just opened,
	// but the dive was not ploted.
	// force a replot by modifying the dive
	// hold by the view, and issuing a plot.
	if (dive && !scene()->items().count()) {
		dive = 0;
		plot(get_dive(selected_dive));
	}
}

void ProfileGraphicsView::clear()
{
	resetTransform();
	zoomLevel = 0;
	if(toolTip){
		scene()->removeItem(toolTip);
		toolTip->deleteLater();
		toolTip = 0;
	}
	scene()->clear();
}

void ProfileGraphicsView::refresh()
{
	clear();
	plot(current_dive, TRUE);
}

void ProfileGraphicsView::setPrintMode(bool mode, bool grayscale)
{
	printMode = mode;
	isGrayscale = grayscale;
}

QColor ProfileGraphicsView::getColor(const color_indice_t i)
{
	return profile_color[i].at((isGrayscale) ? 1 : 0);
}

void ProfileGraphicsView::plot(struct dive *d, bool forceRedraw)
{
	struct divecomputer *dc;

	if (d)
		dc = select_dc(&d->dc);

	if (!forceRedraw && dive == d && (d && dc == diveDC))
		return;

	clear();
	dive = d;
	diveDC = d ? dc : NULL;

	if (!isVisible() || !dive) {
		return;
	}
	setBackgroundBrush(getColor(BACKGROUND));

	// best place to put the focus stealer code.
	setFocusProxy(mainWindow()->dive_list());
	scene()->setSceneRect(0,0, viewport()->width()-50, viewport()->height()-50);

	toolTip = new ToolTipItem();
	installEventFilter(toolTip);
	scene()->addItem(toolTip);
	if (printMode)
		toolTip->setVisible(false);

	// Fix this for printing / screen later.
	// plot_set_scale(scale_mode_t);

	if (!dc->samples) {
		dc = fake_dc(dc);
	}

	QString nick = get_dc_nickname(dc->model, dc->deviceid);
	if (nick.isEmpty())
		nick = QString(dc->model);

	if (nick.isEmpty())
		nick = tr("unknown divecomputer");

	if ( tr("unknown divecomputer") == nick){
		mode = PLAN;
	}else{
		mode = DIVE;
	}

	/*
	 * Set up limits that are independent of
	 * the dive computer
	 */
	calculate_max_limits(dive, dc, &gc);

	QRectF profile_grid_area = scene()->sceneRect();
	gc.maxx = (profile_grid_area.width() - 2 * profile_grid_area.x());
	gc.maxy = (profile_grid_area.height() - 2 * profile_grid_area.y());

	/* This is per-dive-computer */
	gc.pi = *create_plot_info(dive, dc, &gc);

	/* Bounding box */
	QPen pen = defaultPen;
	pen.setColor(getColor(TIME_GRID));
	QGraphicsRectItem *rect = new QGraphicsRectItem(profile_grid_area);
	rect->setPen(pen);
	scene()->addItem(rect);

	/* Depth profile */
	plot_depth_profile();

	plot_events(dc);

	/* Temperature profile */
	plot_temperature_profile();

	/* Cylinder pressure plot */
	plot_cylinder_pressure(dc);

	/* Text on top of all graphs.. */
	plot_temperature_text();

	plot_depth_text();

	plot_cylinder_pressure_text();

	plot_deco_text();

	/* Put the dive computer name in the lower left corner */
	gc.leftx = 0; gc.rightx = 1.0;
	gc.topy = 0; gc.bottomy = 1.0;

	text_render_options_t computer = {DC_TEXT_SIZE, TIME_TEXT, LEFT, TOP};
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
	if (zoomLevel == 0) {
		fitInView(sceneRect());
	}
	toolTip->readPos();

	if(mode == PLAN){
		timeEditor = new GraphicsTextEditor();
		timeEditor->setPlainText( dive->duration.seconds ? QString::number(dive->duration.seconds/60) : tr("Set Duration: 10 minutes"));
		timeEditor->setPos(profile_grid_area.width() - timeEditor->boundingRect().width(), timeMarkers->y());
		timeEditor->document();
		connect(timeEditor, SIGNAL(editingFinished(QString)), this, SLOT(edit_dive_time(QString)));
		scene()->addItem(timeEditor);
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

	QColor c(getColor(DEPTH_GRID));

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
	QColor c = getColor(PP_LINES);

	for (m = 0.0; m <= pp; m += dpp) {
		QGraphicsLineItem *item = new QGraphicsLineItem(SCALEGC(0, m), SCALEGC(hpos, m));
		QPen pen(defaultPen);
		pen.setColor(c);
		item->setPen(pen);
		scene()->addItem(item);
		plot_text(&tro, QPointF(hpos + 30, m), QString::number(m));
	}
}

void ProfileGraphicsView::plot_add_line(int sec, double val, QColor c, QPointF &from)
{
	QPointF to = QPointF(SCALEGC(sec, val));
	QGraphicsLineItem *item = new QGraphicsLineItem(from.x(), from.y(), to.x(), to.y());
	QPen pen(defaultPen);
	pen.setColor(c);
	item->setPen(pen);
	scene()->addItem(item);
	from = to;
}

void ProfileGraphicsView::plot_pp_gas_profile()
{
	int i;
	struct plot_data *entry;
	struct plot_info *pi = &gc.pi;

	setup_pp_limits(&gc);
	QColor c;
	QPointF from, to;
	if (prefs.pp_graphs.pn2) {
		c = getColor(PN2);
		entry = pi->entry;
		from = QPointF(SCALEGC(entry->sec, entry->pn2));
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->pn2 < prefs.pp_graphs.pn2_threshold)
				plot_add_line(entry->sec, entry->pn2, c, from);
			else
				from = QPointF(SCALEGC(entry->sec, entry->pn2));
		}

		c = getColor(PN2_ALERT);
		entry = pi->entry;
		from = QPointF(SCALEGC(entry->sec, entry->pn2));
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->pn2 >= prefs.pp_graphs.pn2_threshold)
				plot_add_line(entry->sec, entry->pn2, c, from);
			else
				from = QPointF(SCALEGC(entry->sec, entry->pn2));
		}
	}

	if (prefs.pp_graphs.phe) {
		c = getColor(PHE);
		entry = pi->entry;

		from = QPointF(SCALEGC(entry->sec, entry->phe));
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->phe < prefs.pp_graphs.phe_threshold)
				plot_add_line(entry->sec, entry->phe, c, from);
			else
				from = QPointF(SCALEGC(entry->sec, entry->phe));
		}

		c = getColor(PHE_ALERT);
		entry = pi->entry;
		from = QPointF(SCALEGC(entry->sec, entry->phe));
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->phe >= prefs.pp_graphs.phe_threshold)
				plot_add_line(entry->sec, entry->phe, c, from);
			else
				from = QPointF(SCALEGC(entry->sec, entry->phe));
		}
	}
	if (prefs.pp_graphs.po2) {
		c = getColor(PO2);
		entry = pi->entry;
		from = QPointF(SCALEGC(entry->sec, entry->po2));
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->po2 < prefs.pp_graphs.po2_threshold)
				plot_add_line(entry->sec, entry->po2, c, from);
			else
				from = QPointF(SCALEGC(entry->sec, entry->po2));
		}

		c = getColor(PO2_ALERT);
		entry = pi->entry;
		from = QPointF(SCALEGC(entry->sec, entry->po2));
		for (i = 1; i < pi->nr; i++) {
			entry++;
			if (entry->po2 >= prefs.pp_graphs.po2_threshold)
				plot_add_line(entry->sec, entry->po2, c, from);
			 else
				from = QPointF(SCALEGC(entry->sec, entry->po2));
		}
	}
}

void ProfileGraphicsView::plot_deco_text()
{
	if (prefs.profile_calc_ceiling) {
		float x = gc.leftx + (gc.rightx - gc.leftx) / 2;
		float y = gc.topy = 1.0;
		static text_render_options_t tro = {PRESSURE_TEXT_SIZE, PRESSURE_TEXT, CENTER, BOTTOM};
		gc.bottomy = 0.0;
		plot_text(&tro, QPointF(x, y), QString("GF %1/%2").arg(prefs.gflow).arg(prefs.gfhigh));
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
				plot_gas_value(mbar, entry->sec, LEFT, TOP,
						get_o2(&dive->cylinder[cyl].gasmix),
						get_he(&dive->cylinder[cyl].gasmix));
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

void ProfileGraphicsView::plot_gas_value(int mbar, int sec, double xalign, double yalign, int o2, int he)
{
	QString gas;
	if (is_air(o2, he))
		gas = tr("air");
	else if (he == 0)
		gas = QString(tr("EAN%1")).arg((o2 + 5) / 10);
	else
		gas = QString("%1/%2").arg((o2 + 5) / 10).arg((he + 5) / 10);
	static text_render_options_t tro = {PRESSURE_TEXT_SIZE, PRESSURE_TEXT, xalign, yalign};
	plot_text(&tro, QPointF(sec, mbar), gas);

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

void ProfileGraphicsView::plot_cylinder_pressure(struct divecomputer *dc)
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
		return getColor((color_indice_t)(SAC_COLORS_START_IDX + sac_index));
	}
	return getColor(SAC_DEFAULT);
}

void ProfileGraphicsView::plot_events(struct divecomputer *dc)
{
	struct event *event = dc->events;

// 	if (gc->printer) {
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

	EventItem *item = new EventItem(0, isGrayscale);
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
		name += ev->flags == SAMPLE_FLAGS_BEGIN ? tr(" begin", "Starts with space!") :
				ev->flags == SAMPLE_FLAGS_END ? tr(" end", "Starts with space!") : "";
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

	QColor c = getColor(TIME_GRID);
	for (i = incr; i < maxtime; i += incr) {
		QGraphicsLineItem *item = new QGraphicsLineItem(SCALEGC(i, 0), SCALEGC(i, 1));
		QPen pen(defaultPen);
		pen.setColor(c);
		item->setPen(pen);
		scene()->addItem(item);
	}

	timeMarkers = new QGraphicsRectItem();
	/* now the text on the time markers */
	struct text_render_options tro = {DEPTH_TEXT_SIZE, TIME_TEXT, CENTER, LINE_DOWN};
	if (maxtime < 600) {
		/* Be a bit more verbose with shorter dives */
		for (i = incr; i < maxtime; i += incr)
			plot_text(&tro, QPointF(i, 0), QString("%1:%2").arg(i/60).arg(i%60, 2, 10, QChar('0')), timeMarkers);
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

	c = getColor(DEPTH_GRID);

	for (i = marker; i < maxline; i += marker) {
		QGraphicsLineItem *item = new QGraphicsLineItem(SCALEGC(0, i), SCALEGC(1, i));
		QPen pen(defaultPen);
		pen.setColor(c);
		item->setPen(pen);
		scene()->addItem(item);
	}

	gc.leftx = 0; gc.rightx = maxtime;
	c = getColor(MEAN_DEPTH);

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
	if (prefs.profile_dc_ceiling) {
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
	}
	pat.setColorAt(1, getColor(DEPTH_BOTTOM));
	pat.setColorAt(0, getColor(DEPTH_TOP));

	neatFill = new QGraphicsPolygonItem();
	neatFill->setPolygon(p);
	neatFill->setBrush(QBrush(pat));
	neatFill->setPen(QPen(QBrush(Qt::transparent),0));
	scene()->addItem(neatFill);


	/* if the user wants the deco ceiling more visible, do that here (this
	 * basically draws over the background that we had allowed to shine
	 * through so far) */
	if (prefs.profile_dc_ceiling && prefs.profile_red_ceiling) {
		p.clear();
		pat.setColorAt(0, getColor(CEILING_SHALLOW));
		pat.setColorAt(1, getColor(CEILING_DEEP));

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
	}

	/* finally, plot the calculated ceiling over all this */
	if (prefs.profile_calc_ceiling) {
		pat.setColorAt(0, getColor(CALC_CEILING_SHALLOW));
		pat.setColorAt(1, getColor(CALC_CEILING_DEEP));

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
	}

	/* plot the calculated ceiling for all tissues */
	if (prefs.profile_calc_ceiling && prefs.calc_all_tissues){
		int k;
		for (k=0; k<16; k++){
			pat.setColorAt(0, getColor(CALC_CEILING_SHALLOW));
			pat.setColorAt(1, QColor(100, 100, 100, 50));

			entry = gc.pi.entry;
			p.clear();
			p.append(QPointF(SCALEGC(0, 0)));
			for (i = 0; i < gc.pi.nr; i++, entry++) {
				if ((entry->ceilings)[k])
					p.append(QPointF(SCALEGC(entry->sec, (entry->ceilings)[k])));
				else
					p.append(QPointF(SCALEGC(entry->sec, 0)));
			}
			p.append(QPointF(SCALEGC((entry-1)->sec, 0)));
			neatFill = new QGraphicsPolygonItem();
			neatFill->setPolygon(p);
			neatFill->setBrush(pat);
			scene()->addItem(neatFill);
		}
	}
	/* next show where we have been bad and crossed the dc's ceiling */
	if (prefs.profile_dc_ceiling) {
		pat.setColorAt(0, getColor(CEILING_SHALLOW));
		pat.setColorAt(1, getColor(CEILING_DEEP));

		entry = gc.pi.entry;
		p.clear();
		p.append(QPointF(SCALEGC(0, 0)));
		for (i = 0; i < gc.pi.nr; i++, entry++)
			p.append(QPointF(SCALEGC(entry->sec, entry->depth)));

		for (i-- , entry--; i >= 0; i--, entry--) {
			if (entry->ndl == 0 && entry->stopdepth > entry->depth) {
				p.append(QPointF(SCALEGC(entry->sec, entry->stopdepth)));
			} else {
				p.append(QPointF(SCALEGC(entry->sec, entry->depth)));
			}
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
		pen.setColor(getColor((color_indice_t)(VELOCITY_COLORS_START_IDX + entry->velocity)));
		item->setPen(pen);
		scene()->addItem(item);
	}
}

QGraphicsItemGroup *ProfileGraphicsView::plot_text(text_render_options_t *tro,const QPointF& pos, const QString& text, QGraphicsItem *parent)
{
	QFont fnt(font());
	QFontMetrics fm(fnt);

	QPointF point(SCALEGC(pos.x(), pos.y())); // This is neded because of the SCALE macro.
	double dx = tro->hpos * (fm.width(text));
	double dy = tro->vpos * (fm.height());

	QGraphicsItemGroup *group = new QGraphicsItemGroup(parent);
	QPainterPath textPath;
	/* addText() uses bottom-left text baseline and the -3 offset is probably slightly off
	 * for different font sizes. */
	textPath.addText(0, fm.height() - 3, fnt, text);
	QPainterPathStroker stroker;
	stroker.setWidth(3);
	QGraphicsPathItem *strokedItem = new QGraphicsPathItem(stroker.createStroke(textPath), group);
	strokedItem->setBrush(QBrush(getColor(TEXT_BACKGROUND)));
	strokedItem->setPen(Qt::NoPen);

	QGraphicsPathItem *textItem = new QGraphicsPathItem(textPath, group);
	textItem->setBrush(QBrush(getColor(tro->color)));
	textItem->setPen(Qt::NoPen);

	group->setPos(point.x() + dx, point.y() + dy);
	if (!printMode)
		group->setFlag(QGraphicsItem::ItemIgnoresTransformations);

	if (!parent)
		scene()->addItem(group);
	return group;
}

void ProfileGraphicsView::resizeEvent(QResizeEvent *event)
{
	fitInView(sceneRect());
}

void ProfileGraphicsView::plot_temperature_profile()
{
	int last = 0;

	if (!setup_temperature_limits(&gc))
		return;

	QPointF from;
	QPointF to;
	QColor color = getColor(TEMP_PLOT);

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
		} else {
			from = QPointF(SCALEGC(sec, mkelvin));
		}
		last = mkelvin;
	}
}

void ProfileGraphicsView::edit_dive_time(const QString& time)
{
	// this should set the full time of the dive.
	refresh();
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
	toolTips.push_back(qMakePair(iconItem, textItem));
	expand();
}

void ToolTipItem::refresh(struct graphics_context *gc, QPointF pos)
{
	clear();
	int time = (pos.x() * gc->maxtime) / gc->maxx;
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
	animation->setStartValue(nextRectangle);
	animation->setEndValue(QRect(0, 0, ICON_SMALL, ICON_SMALL));
	animation->start(QAbstractAnimation::DeleteWhenStopped);
	clear();

	status = COLLAPSED;
}

void ToolTipItem::expand()
{

	if (!title) {
		return;
	}

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

	if (height < ICON_SMALL)
		height = ICON_SMALL;

	nextRectangle.setWidth(width);
	nextRectangle.setHeight(height);

	QPropertyAnimation *animation = new QPropertyAnimation(this, "rect");
	animation->setDuration(100);
	animation->setStartValue(rectangle);
	animation->setEndValue(nextRectangle);
	animation->start(QAbstractAnimation::DeleteWhenStopped);

	status = EXPANDED;
}

ToolTipItem::ToolTipItem(QGraphicsItem* parent): QGraphicsPathItem(parent), background(0)
{
	title = new QGraphicsSimpleTextItem(tr("Information"), this);
	separator = new QGraphicsLineItem(this);
	dragging = false;
	setFlag(ItemIgnoresTransformations);
	status = COLLAPSED;
	updateTitlePosition();
	setZValue(99);
}

ToolTipItem::~ToolTipItem()
{
	clear();
}


void ToolTipItem::updateTitlePosition()
{
	if (rectangle.width() < title->boundingRect().width() + SPACING*4) {
		QRectF newRect = rectangle;
		newRect.setWidth(title->boundingRect().width() + SPACING*4);
		newRect.setHeight((newRect.height() && isExpanded()) ? newRect.height() : ICON_SMALL);
		setRect(newRect);
	}

	title->setPos(boundingRect().width()/2  - title->boundingRect().width()/2 -1, 0);
	title->setFlag(ItemIgnoresTransformations);
	title->setPen(QPen(Qt::white, 1));
	title->setBrush(Qt::white);

	if (toolTips.size() > 0) {
		double x1 = 3;
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

bool ToolTipItem::isExpanded() {
	return status == EXPANDED;
}

void ToolTipItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
	persistPos();
	dragging = false;
}

void ToolTipItem::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
	dragging = true;
}

void ToolTipItem::persistPos()
{
	QPoint currentPos = scene()->views().at(0)->mapFromScene(pos());
	QSettings s;
	s.beginGroup("ProfileMap");
	s.setValue("tooltip_position", currentPos);
	s.endGroup();
	s.sync();
}

void ToolTipItem::readPos()
{
	QSettings s;
	s.beginGroup("ProfileMap");
	QPointF value = scene()->views().at(0)->mapToScene(
		s.value("tooltip_position").toPoint()
	);
	setPos(value);
}

bool ToolTipItem::eventFilter(QObject* view, QEvent* event)
{
	if (event->type() == QEvent::HoverMove && dragging){
		QHoverEvent *e = static_cast<QHoverEvent*>(event);
		QGraphicsView *v = scene()->views().at(0);
		setPos( v->mapToScene(e->pos()));
	}
	return false;
}

QColor EventItem::getColor(const color_indice_t i)
{
	return profile_color[i].at((isGrayscale) ? 1 : 0);
}

EventItem::EventItem(QGraphicsItem* parent, bool grayscale): QGraphicsPolygonItem(parent)
{
	isGrayscale = grayscale;
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
	pen.setBrush(QBrush(getColor(ALERT_BG)));

	setPolygon(poly);
	setBrush(QBrush(getColor(ALERT_BG)));
	setPen(pen);

	QGraphicsLineItem *line = new QGraphicsLineItem(0, 5, 0, 10, this);
	line->setPen(QPen(getColor(ALERT_FG), 2));

	QGraphicsEllipseItem *ball = new QGraphicsEllipseItem(-1, 12, 2, 2, this);
	ball->setBrush(QBrush(getColor(ALERT_FG)));
	ball->setPen(QPen(getColor(ALERT_FG)));
}

GraphicsTextEditor::GraphicsTextEditor(QGraphicsItem* parent): QGraphicsTextItem(parent)
{
}

void GraphicsTextEditor::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
	// Remove the proxy filter so we can focus here.
	mainWindow()->graphics()->setFocusProxy(0);
	setTextInteractionFlags(Qt::TextEditorInteraction | Qt::TextEditable);
}

void GraphicsTextEditor::keyReleaseEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Enter || event->key() == Qt::Key_Return){
		setTextInteractionFlags(Qt::NoTextInteraction);
		emit editingFinished( toPlainText() );
		mainWindow()->graphics()->setFocusProxy(mainWindow()->dive_list());
		return;
	}
	emit textChanged( toPlainText() );
    QGraphicsTextItem::keyReleaseEvent(event);
}
