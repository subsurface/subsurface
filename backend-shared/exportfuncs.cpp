// SPDX-License-Identifier: GPL-2.0
#include "exportfuncs.h"
#include "core/membuffer.h"
#include "core/dive.h"
#include "core/divelog.h"
#include "core/divesite.h"
#include "core/gettextfromc.h"
#include "core/tag.h"
#include "core/subsurface-time.h"
#include "core/file.h"
#include "core/errorhelper.h"
#include "core/divefilter.h"
#include "core/divesite.h"
#include "core/picture.h"
#include "core/pref.h"
#include "core/sample.h"
#include "core/selection.h"
#include "core/taxonomy.h"
#include "core/sample.h"
#include "profile-widget/profilescene.h"
#include <QDir>
#include <QFileInfo>
#include <QtConcurrent>
#include <memory>

// Default implementation of the export callback: do nothing / never cancel
void ExportCallback::setProgress(int)
{
}

bool ExportCallback::canceled() const
{
	return false;
}

#if !defined(SUBSURFACE_MOBILE)

// Let's say that 800x600 is a "reasonable" profile size. Use four times that for printing.
static constexpr int profileScale = 4;
static constexpr int profileWidth = 800 * profileScale;
static constexpr int profileHeight = 600 * profileScale;

static void exportProfile(ProfileScene *profile, const struct dive *dive, const QString &filename)
{
	QImage image = QImage(QSize(profileWidth, profileHeight), QImage::Format_RGB32);
	QPainter paint;
	paint.begin(&image);
	profile->draw(&paint, QRect(0, 0, profileWidth, profileHeight), dive, 0, nullptr, false);
	image.save(filename);
}

static std::unique_ptr<ProfileScene> getPrintProfile()
{
	return std::make_unique<ProfileScene>((double)profileScale, true, false);
}

void exportProfile(QString filename, bool selected_only, ExportCallback &cb)
{
	struct dive *dive;
	int i;
	int count = 0;
	if (!filename.endsWith(".png", Qt::CaseInsensitive))
		filename = filename.append(".png");
	QFileInfo fi(filename);

	int todo = selected_only ? amount_selected : divelog.dives->nr;
	int done = 0;
	auto profile = getPrintProfile();
	for_each_dive (i, dive) {
		if (cb.canceled())
			return;
		if (selected_only && !dive->selected)
			continue;
		cb.setProgress(done++ * 1000 / todo);
		QString fn = count ? fi.path() + QDir::separator() + fi.completeBaseName().append(QString("-%1.").arg(count)) + fi.suffix()
				   : filename;
		exportProfile(profile.get(), dive, fn);
		++count;
	}
}

void export_TeX(const char *filename, bool selected_only, bool plain, ExportCallback &cb)
{
	FILE *f;
	QDir texdir = QFileInfo(filename).dir();
	struct dive *dive;
	const struct units *units = get_units();
	const char *unit;
	const char *ssrf;
	int i;
	bool need_pagebreak = false;

	struct membufferpp buf;

	if (plain) {
		ssrf = "";
		put_format(&buf, "\\input subsurfacetemplate\n");
		put_format(&buf, "%% This is a plain TeX file. Compile with pdftex, not pdflatex!\n");
		put_format(&buf, "%% You will also need a subsurfacetemplate.tex in the current directory.\n");
	} else {
		ssrf = "ssrf";
		put_format(&buf, "\\input subsurfacelatextemplate\n");
		put_format(&buf, "%% This is a plain LaTeX file. Compile with pdflatex, not pdftex!\n");
		put_format(&buf, "%% You will also need a subsurfacelatextemplate.tex in the current directory.\n");
	}
	put_format(&buf, "%% You can download an example from http://www.atdotde.de/~robert/subsurfacetemplate\n%%\n");
	put_format(&buf, "%%\n");
	put_format(&buf, "%% Notes: TeX/LaTex will not render the degree symbol correctly by default. In LaTeX, you may\n");
	put_format(&buf, "%% add the following line to the end of the preamble of your template to ensure correct output:\n");
	put_format(&buf, "%% \\usepackage[utf8]{inputenc}\n");
	put_format(&buf, "%% \\usepackage{gensymb}\n");
	put_format(&buf, "%% \\DeclareUnicodeCharacter{00B0}{\\degree}\n"); //replaces ° with \degree
	put_format(&buf, "%%\n");

	/* Define text fields with the units used for export.  These values are set in the Subsurface Preferences
	 * and the text fields created here are included in the data fields below.
	 */
	put_format(&buf, "\n%% These fields contain the units used in other fields below. They may be\n");
	put_format(&buf, "%% referenced as needed in TeX templates.\n");
	put_format(&buf, "%% \n");
	put_format(&buf, "%% By default, Subsurface exports units of volume as \"ℓ\" and \"cuft\", which do\n");
	put_format(&buf, "%% not render well in TeX/LaTeX.  The code below substitutes \"L\" and \"ft$^{3}$\",\n");
	put_format(&buf, "%% respectively.  If you wish to display the original values, you may edit this\n");
	put_format(&buf, "%% list and all calls to those units will be updated in your document.\n");

	put_format(&buf, "\\def\\%sdepthunit{\\%sunit%s}", ssrf, ssrf, units->length == units::METERS ? "meter" : "ft");
	put_format(&buf, "\\def\\%sweightunit{\\%sunit%s}",ssrf, ssrf, units->weight == units::KG ? "kg" : "lb");
	put_format(&buf, "\\def\\%spressureunit{\\%sunit%s}", ssrf, ssrf, units->pressure == units::BAR ? "bar" : "psi");
	put_format(&buf, "\\def\\%stemperatureunit{\\%sunit%s}", ssrf, ssrf, units->temperature == units::CELSIUS ? "centigrade" : "fahrenheit");
	put_format(&buf, "\\def\\%svolumeunit{\\%sunit%s}", ssrf, ssrf, units->volume == units::LITER ? "liter" : "cuft");
	put_format(&buf, "\\def\\%sverticalspeedunit{\\%sunit%s}", ssrf, ssrf, units->length == units::METERS ? "meterpermin" : "ftpermin");

	put_format(&buf, "\n%%%%%%%%%% Begin Dive Data: %%%%%%%%%%\n");

	int todo = selected_only ? amount_selected : divelog.dives->nr;
	int done = 0;
	auto profile = getPrintProfile();
	for_each_dive (i, dive) {
		if (cb.canceled())
			return;
		if (selected_only && !dive->selected)
			continue;
		cb.setProgress(done++ * 1000 / todo);
		exportProfile(profile.get(), dive, texdir.filePath(QString("profile%1.png").arg(dive->number)));
		struct tm tm;
		utc_mkdate(dive->when, &tm);

		const char *country = NULL;
		dive_site *site = dive->dive_site;
		if (site)
			country = taxonomy_get_country(&site->taxonomy);
		pressure_t delta_p = {.mbar = 0};

		QString star = "*";
		QString viz = star.repeated(dive->visibility);
		QString rating = star.repeated(dive->rating);

		int i;
		int qty_cyl;
		int qty_weight;
		double total_weight;

		if (need_pagebreak) {
			if (plain)
				put_format(&buf, "\\vfill\\eject\n");

			else
				put_format(&buf, "\\newpage\n");
		}
		need_pagebreak = true;
		put_format(&buf, "\n%% Time, Date, and location:\n");
		put_format(&buf, "\\def\\%sdate{%04u-%02u-%02u}\n", ssrf,
		      tm.tm_year, tm.tm_mon+1, tm.tm_mday);
		put_format(&buf, "\\def\\%shour{%02u}\n", ssrf, tm.tm_hour);
		put_format(&buf, "\\def\\%sminute{%02u}\n", ssrf, tm.tm_min);
		put_format(&buf, "\\def\\%snumber{%d}\n", ssrf, dive->number);
		put_format(&buf, "\\def\\%splace{%s}\n", ssrf, site ? site->name : "");
		put_format(&buf, "\\def\\%sspot{}\n", ssrf);
		put_format(&buf, "\\def\\%ssitename{%s}\n", ssrf, site ? site->name : "");
		site ? put_format(&buf, "\\def\\%sgpslat{%f}\n", ssrf, site->location.lat.udeg / 1000000.0) : put_format(&buf, "\\def\\%sgpslat{}\n", ssrf);
		site ? put_format(&buf, "\\def\\%sgpslon{%f}\n", ssrf, site->location.lon.udeg / 1000000.0) : put_format(&buf, "\\def\\gpslon{}\n");
		put_format(&buf, "\\def\\%scomputer{%s}\n", ssrf, dive->dc.model);
		put_format(&buf, "\\def\\%scountry{%s}\n", ssrf, country ?: "");
		put_format(&buf, "\\def\\%stime{%u:%02u}\n", ssrf, FRACTION(dive->duration.seconds, 60));

		put_format(&buf, "\n%% Dive Profile Details:\n");
		dive->maxtemp.mkelvin ? put_format(&buf, "\\def\\%smaxtemp{%.1f\\%stemperatureunit}\n", ssrf, get_temp_units(dive->maxtemp.mkelvin, &unit), ssrf) : put_format(&buf, "\\def\\%smaxtemp{}\n", ssrf);
		dive->mintemp.mkelvin ? put_format(&buf, "\\def\\%smintemp{%.1f\\%stemperatureunit}\n", ssrf, get_temp_units(dive->mintemp.mkelvin, &unit), ssrf) : put_format(&buf, "\\def\\%ssrfmintemp{}\n", ssrf);
		dive->watertemp.mkelvin ? put_format(&buf, "\\def\\%swatertemp{%.1f\\%stemperatureunit}\n", ssrf, get_temp_units(dive->watertemp.mkelvin, &unit), ssrf) : put_format(&buf, "\\def\\%swatertemp{}\n", ssrf);
		dive->airtemp.mkelvin ? put_format(&buf, "\\def\\%sairtemp{%.1f\\%stemperatureunit}\n", ssrf, get_temp_units(dive->airtemp.mkelvin, &unit), ssrf) : put_format(&buf, "\\def\\%sairtemp{}\n", ssrf);
		dive->maxdepth.mm ? put_format(&buf, "\\def\\%smaximumdepth{%.1f\\%sdepthunit}\n", ssrf, get_depth_units(dive->maxdepth.mm, NULL, &unit), ssrf) : put_format(&buf, "\\def\\%smaximumdepth{}\n", ssrf);
		dive->meandepth.mm ? put_format(&buf, "\\def\\%smeandepth{%.1f\\%sdepthunit}\n", ssrf, get_depth_units(dive->meandepth.mm, NULL, &unit), ssrf) : put_format(&buf, "\\def\\%smeandepth{}\n", ssrf);

		struct tag_entry *tag = dive->tag_list;
		QString tags;
		if (tag) {
			tags = tag->tag->name;
			while ((tag = tag->next))
				tags += QString(", ") + QString(tag->tag->name);
		}
		put_format(&buf, "\\def\\%stype{%s}\n", ssrf, qPrintable(tags));
		put_format(&buf, "\\def\\%sviz{%s}\n", ssrf, qPrintable(viz));
		put_format(&buf, "\\def\\%srating{%s}\n", ssrf, qPrintable(rating));
		put_format(&buf, "\\def\\%splot{\\includegraphics[width=9cm,height=4cm]{profile%d}}\n", ssrf, dive->number);
		put_format(&buf, "\\def\\%sprofilename{profile%d}\n", ssrf, dive->number);
		put_format(&buf, "\\def\\%scomment{%s}\n", ssrf, dive->notes ? dive->notes : "");
		put_format(&buf, "\\def\\%sbuddy{%s}\n", ssrf, dive->buddy ? dive->buddy : "");
		put_format(&buf, "\\def\\%sdivemaster{%s}\n", ssrf, dive->diveguide ? dive->diveguide : "");
		put_format(&buf, "\\def\\%ssuit{%s}\n", ssrf, dive->suit ? dive->suit : "");

		// Print cylinder data
		put_format(&buf, "\n%% Gas use information:\n");
		qty_cyl = 0;
		for (i = 0; i < dive->cylinders.nr; i++){
			const cylinder_t &cyl = *get_cylinder(dive, i);
			if (is_cylinder_used(dive, i) || (prefs.include_unused_tanks && cyl.type.description)){
				put_format(&buf, "\\def\\%scyl%cdescription{%s}\n", ssrf, 'a' + i, cyl.type.description);
				put_format(&buf, "\\def\\%scyl%cgasname{%s}\n", ssrf, 'a' + i, gasname(cyl.gasmix));
				put_format(&buf, "\\def\\%scyl%cmixO2{%.1f\\%%}\n", ssrf, 'a' + i, get_o2(cyl.gasmix)/10.0);
				put_format(&buf, "\\def\\%scyl%cmixHe{%.1f\\%%}\n", ssrf, 'a' + i, get_he(cyl.gasmix)/10.0);
				put_format(&buf, "\\def\\%scyl%cmixN2{%.1f\\%%}\n", ssrf, 'a' + i, (100.0 - (get_o2(cyl.gasmix)/10.0) - (get_he(cyl.gasmix)/10.0)));
				delta_p.mbar += cyl.start.mbar - cyl.end.mbar;
				put_format(&buf, "\\def\\%scyl%cstartpress{%.1f\\%spressureunit}\n", ssrf, 'a' + i, get_pressure_units(cyl.start.mbar, &unit)/1.0, ssrf);
				put_format(&buf, "\\def\\%scyl%cendpress{%.1f\\%spressureunit}\n", ssrf, 'a' + i, get_pressure_units(cyl.end.mbar, &unit)/1.0, ssrf);
				qty_cyl += 1;
			} else {
				put_format(&buf, "\\def\\%scyl%cdescription{}\n", ssrf, 'a' + i);
				put_format(&buf, "\\def\\%scyl%cgasname{}\n", ssrf, 'a' + i);
				put_format(&buf, "\\def\\%scyl%cmixO2{}\n", ssrf, 'a' + i);
				put_format(&buf, "\\def\\%scyl%cmixHe{}\n", ssrf, 'a' + i);
				put_format(&buf, "\\def\\%scyl%cmixN2{}\n", ssrf, 'a' + i);
				delta_p.mbar += cyl.start.mbar - cyl.end.mbar;
				put_format(&buf, "\\def\\%scyl%cstartpress{}\n", ssrf, 'a' + i);
				put_format(&buf, "\\def\\%scyl%cendpress{}\n", ssrf, 'a' + i);
				qty_cyl += 1;
			}
		}
		put_format(&buf, "\\def\\%sqtycyl{%d}\n", ssrf, qty_cyl);
		put_format(&buf, "\\def\\%sgasuse{%.1f\\%spressureunit}\n", ssrf, get_pressure_units(delta_p.mbar, &unit)/1.0, ssrf);
		put_format(&buf, "\\def\\%ssac{%.2f\\%svolumeunit/min}\n", ssrf, get_volume_units(dive->sac, NULL, &unit), ssrf);

		//Code block prints all weights listed in dive.
		put_format(&buf, "\n%% Weighting information:\n");
		qty_weight = 0;
		total_weight = 0;
		for (i = 0; i < dive->weightsystems.nr; i++) {
			weightsystem_t w = dive->weightsystems.weightsystems[i];
			put_format(&buf, "\\def\\%sweight%ctype{%s}\n", ssrf, 'a' + i, w.description);
			put_format(&buf, "\\def\\%sweight%camt{%.3f\\%sweightunit}\n", ssrf, 'a' + i, get_weight_units(w.weight.grams, NULL, &unit), ssrf);
			qty_weight += 1;
			total_weight += get_weight_units(w.weight.grams, NULL, &unit);
		}
		put_format(&buf, "\\def\\%sqtyweights{%d}\n", ssrf, qty_weight);
		put_format(&buf, "\\def\\%stotalweight{%.2f\\%sweightunit}\n", ssrf, total_weight, ssrf);
		unit = "";

		// Legacy fields
		put_format(&buf, "\\def\\%sspot{}\n", ssrf);
		put_format(&buf, "\\def\\%sentrance{}\n", ssrf);
		put_format(&buf, "\\def\\%splace{%s}\n", ssrf, site ? site->name : "");
		dive->maxdepth.mm ? put_format(&buf, "\\def\\%sdepth{%.1f\\%sdepthunit}\n", ssrf, get_depth_units(dive->maxdepth.mm, NULL, &unit), ssrf) : put_format(&buf, "\\def\\%sdepth{}\n", ssrf);

		put_format(&buf, "\\%spage\n", ssrf);
	}

	if (plain)
		put_format(&buf, "\\bye\n");
	else
		put_format(&buf, "\\end{document}\n");

	f = subsurface_fopen(filename, "w+");
	if (!f) {
		report_error(qPrintable(gettextFromC::tr("Can't open file %s")), filename);
	} else {
		flush_buffer(&buf, f); /*check for writing errors? */
		fclose(f);
	}
	cb.setProgress(1000);
}

void export_depths(const char *filename, bool selected_only)
{
	FILE *f;
	struct dive *dive;
	depth_t depth;
	int i;
	const char *unit = NULL;

	struct membufferpp buf;

	for_each_dive (i, dive) {
		if (selected_only && !dive->selected)
			continue;

		FOR_EACH_PICTURE (dive) {
			int n = dive->dc.samples;
			struct sample *s = dive->dc.sample;
			depth.mm = 0;
			while (--n >= 0 && (int32_t)s->time.seconds <= picture->offset.seconds) {
				depth.mm = s->depth.mm;
				s++;
			}
			put_format(&buf, "%s\t%.1f", picture->filename, get_depth_units(depth.mm, NULL, &unit));
			put_format(&buf, "%s\n", unit);
		}
	}

	f = subsurface_fopen(filename, "w+");
	if (!f) {
		report_error(qPrintable(gettextFromC::tr("Can't open file %s")), filename);
	} else {
		flush_buffer(&buf, f); /*check for writing errors? */
		fclose(f);
	}
}
#endif /* ! SUBSURFACE_MOBILE */

std::vector<const dive_site *> getDiveSitesToExport(bool selectedOnly)
{
	std::vector<const dive_site *> res;
#ifndef SUBSURFACE_MOBILE
	// Waiting for DiveFilter to be combined for both mobile and desktop

	if (selectedOnly && DiveFilter::instance()->diveSiteMode()) {
		// Special case in dive site mode: export all selected dive sites,
		// not the dive sites of selected dives.
		QVector<dive_site *> sites = DiveFilter::instance()->filteredDiveSites();
		res.reserve(sites.size());
		for (const dive_site *ds: sites)
			res.push_back(ds);
		return res;
	}

	res.reserve(divelog.sites->nr);
	for (int i = 0; i < divelog.sites->nr; i++) {
		struct dive_site *ds = get_dive_site(i, divelog.sites);
		if (dive_site_is_empty(ds))
			continue;
		if (selectedOnly && !is_dive_site_selected(ds))
			continue;
		res.push_back(ds);
	}
#else
	/* walk the dive site list */
	int i;
	const struct dive_site *ds;
	for_each_dive_site (i, ds, divelog.sites)
		res.push_back(get_dive_site(i, divelog.sites));
#endif
	return res;
}

QFuture<int> exportUsingStyleSheet(const QString &filename, bool doExport, int units,
	const QString &stylesheet, bool anonymize)
{
	return QtConcurrent::run(export_dives_xslt, filename.toUtf8(), doExport, units, stylesheet.toUtf8(), anonymize);
}
