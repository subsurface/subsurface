// SPDX-License-Identifier: GPL-2.0
/* planner.c
 *
 * code that allows us to plan future dives
 *
 * (c) Dirk Hohndel 2013
 */
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "dive.h"
#include "deco.h"
#include "units.h"
#include "divelist.h"
#include "planner.h"
#include "gettext.h"
#include "libdivecomputer/parser.h"
#include "qthelper.h"
#include "format.h"
#include "version.h"
#include "membuffer.h"

static int diveplan_duration(struct diveplan *diveplan)
{
	struct divedatapoint *dp = diveplan->dp;
	int duration = 0;
	int lastdepth = 0;
	while(dp) {
		if (dp->time > duration && (dp->depth.mm > SURFACE_THRESHOLD || lastdepth > SURFACE_THRESHOLD)) {
			duration = dp->time;
			lastdepth = dp->depth.mm;
		}
		dp = dp->next;
	}
	return (duration + 30) / 60;
}

/* Add the icd results of one trimix gas change to the dive plan html buffer. Two rows are added to the table, one
 * indicating fractions of gas, the other indication partial pressures of gas. This function makes use of the
 * icd_data structure that was filled with information by the function isobaric_counterdiffusion().
 * Parameters: 1) Pointer to the output buffer position at which writing should start.
 *             2) The size of the part of the unused output buffer that remains unused.
 *             3) The data structure containing icd calculation results: icdvalues.
 *             4) A boolean value indicating whether a table header should be written before the data. Only during 1st call.
 *             5) Pointers to gas mixes in the gas change: gas-from and gas-to.
 * Returns:    The size of the output buffer that has been used after the new results have been added.
 */
static void add_icd_entry(struct membuffer *b, struct icd_data *icdvalues, bool printheader, int time_seconds, int ambientpressure_mbar, struct gasmix gas_from, struct gasmix gas_to)
{
	if (printheader) { // Create a table description and a table header if no icd data have been written yet.
		put_format(b, "<div>%s:", translate("gettextFromC","Isobaric counterdiffusion information"));
		put_format(b, "<table><tr><td align='left'><b>%s</b></td>", translate("gettextFromC", "runtime"));
		put_format(b, "<td align='center'><b>%s</b></td>", translate("gettextFromC", "gaschange"));
		put_format(b, "<td style='padding-left: 15px;'><b>%s</b></td>", translate("gettextFromC", "&#916;He"));
		put_format(b, "<td style='padding-left: 20px;'><b>%s</b></td>", translate("gettextFromC", "&#916;N&#8322;"));
		put_format(b, "<td style='padding-left: 10px;'><b>%s</b></td></tr>", translate("gettextFromC", "max &#916;N&#8322;"));
	}		// Add one entry to the icd table:
	put_format_loc(b,
		"<tr><td rowspan='2' style= 'vertical-align:top;'>%3d%s</td>"
		"<td rowspan=2 style= 'vertical-align:top;'>%s&#10137;",
		(time_seconds + 30) / 60, translate("gettextFromC", "min"), gasname(gas_from));
	put_format_loc(b,
		"%s</td><td style='padding-left: 10px;'>%+5.1f%%</td>"
		"<td style= 'padding-left: 15px; color:%s;'>%+5.1f%%</td>"
		"<td style='padding-left: 15px;'>%+5.1f%%</td></tr>"
		"<tr><td style='padding-left: 10px;'>%+5.2f%s</td>"
		"<td style='padding-left: 15px; color:%s;'>%+5.2f%s</td>"
		"<td style='padding-left: 15px;'>%+5.2f%s</td></tr>",
		gasname(gas_to), icdvalues->dHe / 10.0,
		((5 * icdvalues->dN2) > -icdvalues->dHe) ? "red" : "#383838", icdvalues->dN2 / 10.0 , 0.2 * (-icdvalues->dHe / 10.0),
		ambientpressure_mbar * icdvalues->dHe / 1e6f, translate("gettextFromC", "bar"), ((5 * icdvalues->dN2) > -icdvalues->dHe) ? "red" : "#383838",
		ambientpressure_mbar * icdvalues->dN2 / 1e6f, translate("gettextFromC", "bar"),
		ambientpressure_mbar * -icdvalues->dHe / 5e6f, translate("gettextFromC", "bar"));
}

const char *get_planner_disclaimer()
{
	return translate("gettextFromC", "DISCLAIMER / WARNING: THIS IMPLEMENTATION OF THE %s "
			 "ALGORITHM AND A DIVE PLANNER IMPLEMENTATION BASED ON THAT HAS "
			 "RECEIVED ONLY A LIMITED AMOUNT OF TESTING. WE STRONGLY RECOMMEND NOT TO "
			 "PLAN DIVES SIMPLY BASED ON THE RESULTS GIVEN HERE.");
}

/* Returns newly allocated buffer. Must be freed by caller */
char *get_planner_disclaimer_formatted()
{
	struct membuffer buf = { 0 };
	const char *deco = decoMode(true) == VPMB ? translate("gettextFromC", "VPM-B")
						  : translate("gettextFromC", "BUHLMANN");
	put_format(&buf, get_planner_disclaimer(), deco);
	return detach_cstring(&buf);
}

void add_plan_to_notes(struct diveplan *diveplan, struct dive *dive, bool show_disclaimer, int error)
{
	struct membuffer buf = { 0 };
	struct membuffer icdbuf = { 0 };
	const char *segmentsymbol;
	int lastdepth = 0, lasttime = 0, lastsetpoint = -1, newdepth = 0, lastprintdepth = 0, lastprintsetpoint = -1;
	struct gasmix lastprintgasmix = gasmix_invalid;
	struct divedatapoint *dp = diveplan->dp;
	bool plan_verbatim = prefs.verbatim_plan;
	bool plan_display_runtime = prefs.display_runtime;
	bool plan_display_duration = prefs.display_duration;
	bool plan_display_transitions = prefs.display_transitions;
	bool gaschange_after = !plan_verbatim;
	bool gaschange_before;
	bool rebreatherchange_after = !plan_verbatim;
	bool rebreatherchange_before;
	enum divemode_t lastdivemode = UNDEF_COMP_TYPE;
	bool lastentered = true;
	bool icdwarning = false, icdtableheader = true;
	struct divedatapoint *nextdp = NULL;
	struct divedatapoint *lastbottomdp = NULL;
	struct icd_data icdvalues;
	char *temp;

	if (!dp)
		return;

	if (error) {
		put_format(&buf, "<span style='color: red;'>%s </span> %s<br/>",
				translate("gettextFromC", "Warning:"),
				translate("gettextFromC", "Decompression calculation aborted due to excessive time"));
		goto finished;
	}

	if (show_disclaimer) {
		char *disclaimer = get_planner_disclaimer_formatted();
		put_string(&buf, "<div><b>");
		put_string(&buf, disclaimer);
		put_string(&buf, "</b><br/>\n</div>\n");
		free(disclaimer);
	}

	put_string(&buf, "<div>\n<b>");
	if (diveplan->surface_interval < 0) {
		put_format(&buf, "%s (%s) %s",
			translate("gettextFromC", "Subsurface"),
			subsurface_canonical_version(),
			translate("gettextFromC", "dive plan</b> (overlapping dives detected)"));
		goto finished;
	} else if (diveplan->surface_interval >= 48 * 60 *60) {
		char *current_date = get_current_date();
		put_format(&buf, "%s (%s) %s %s",
			translate("gettextFromC", "Subsurface"),
			subsurface_canonical_version(),
			translate("gettextFromC", "dive plan</b> created on"),
			current_date);
		free(current_date);
	} else {
		char *current_date = get_current_date();
		put_format_loc(&buf, "%s (%s) %s %d:%02d) %s %s",
			translate("gettextFromC", "Subsurface"),
			subsurface_canonical_version(),
			translate("gettextFromC", "dive plan</b> (surface interval "),
			FRACTION(diveplan->surface_interval / 60, 60),
			translate("gettextFromC", "created on"),
			current_date);
		free(current_date);
	}
	put_string(&buf, "<br/>\n");

	if (prefs.display_variations && decoMode(true) != RECREATIONAL)
		put_format_loc(&buf, translate("gettextFromC", "Runtime: %dmin%s"),
			diveplan_duration(diveplan), "VARIATIONS");
	else
		put_format_loc(&buf, translate("gettextFromC", "Runtime: %dmin%s"),
			diveplan_duration(diveplan), "");
	put_string(&buf, "<br/>\n</div>\n");

	if (!plan_verbatim) {
		put_format(&buf, "<table>\n<thead>\n<tr><th></th><th>%s</th>", translate("gettextFromC", "depth"));
		if (plan_display_duration)
			put_format(&buf, "<th style='padding-left: 10px;'>%s</th>", translate("gettextFromC", "duration"));
		if (plan_display_runtime)
			put_format(&buf, "<th style='padding-left: 10px;'>%s</th>", translate("gettextFromC", "runtime"));
		put_format(&buf, "<th style='padding-left: 10px; float: left;'>%s</th></tr>\n</thead>\n<tbody style='float: left;'>\n",
				translate("gettextFromC", "gas"));
	}

	do {
		struct gasmix gasmix, newgasmix = {};
		const char *depth_unit;
		double depthvalue;
		int decimals;
		bool isascent = (dp->depth.mm < lastdepth);

		nextdp = dp->next;
		if (dp->time == 0)
			continue;
		gasmix = get_cylinder(dive, dp->cylinderid)->gasmix;
		depthvalue = get_depth_units(dp->depth.mm, &decimals, &depth_unit);
		/* analyze the dive points ahead */
		while (nextdp && nextdp->time == 0)
			nextdp = nextdp->next;
		if (nextdp)
			newgasmix = get_cylinder(dive, nextdp->cylinderid)->gasmix;
		gaschange_after = (nextdp && (gasmix_distance(gasmix, newgasmix)));
		gaschange_before =  (gasmix_distance(lastprintgasmix, gasmix));
		rebreatherchange_after = (nextdp && (dp->setpoint != nextdp->setpoint || dp->divemode != nextdp->divemode));
		rebreatherchange_before = lastprintsetpoint != dp->setpoint || lastdivemode != dp->divemode;
		/* do we want to skip this leg as it is devoid of anything useful? */
		if (!dp->entered &&
		    nextdp &&
		    dp->depth.mm != lastdepth &&
		    nextdp->depth.mm != dp->depth.mm &&
		    !gaschange_before &&
		    !gaschange_after &&
		    !rebreatherchange_before &&
		    !rebreatherchange_after)
			continue;
		// Ignore final surface segment for notes
		if (lastdepth == 0 && dp->depth.mm == 0 && !dp->next)
			continue;
		if ((dp->time - lasttime < 10 && lastdepth == dp->depth.mm) && !(gaschange_after && dp->next && dp->depth.mm != dp->next->depth.mm))
			continue;

		/* Store pointer to last entered datapoint for minimum gas calculation */
		if (dp->entered && !nextdp->entered)
			lastbottomdp = dp;

		if (plan_verbatim) {
			/* When displaying a verbatim plan, we output a waypoint for every gas change.
			 * Therefore, we do not need to test for difficult cases that mean we need to
			 * print a segment just so we don't miss a gas change.  This makes the logic
			 * to determine whether or not to print a segment much simpler than  with the
			 * non-verbatim plan.
			 */
			if (dp->depth.mm != lastprintdepth) {
				if (plan_display_transitions || dp->entered || !dp->next || (gaschange_after && dp->next && dp->depth.mm != nextdp->depth.mm)) {
					if (dp->setpoint) {
						put_format_loc(&buf, translate("gettextFromC", "%s to %.*f %s in %d:%02d min - runtime %d:%02u on %s (SP = %.1fbar)"),
							     dp->depth.mm < lastprintdepth ? translate("gettextFromC", "Ascend") : translate("gettextFromC", "Descend"),
							     decimals, depthvalue, depth_unit,
							     FRACTION(dp->time - lasttime, 60),
							     FRACTION(dp->time, 60),
							     gasname(gasmix),
							     (double) dp->setpoint / 1000.0);
					} else {
						put_format_loc(&buf, translate("gettextFromC", "%s to %.*f %s in %d:%02d min - runtime %d:%02u on %s"),
							     dp->depth.mm < lastprintdepth ? translate("gettextFromC", "Ascend") : translate("gettextFromC", "Descend"),
							     decimals, depthvalue, depth_unit,
							     FRACTION(dp->time - lasttime, 60),
							     FRACTION(dp->time, 60),
							     gasname(gasmix));
					}

					put_string(&buf, "<br/>\n");
				}
				newdepth = dp->depth.mm;
				lasttime = dp->time;
			} else {
				if ((nextdp && dp->depth.mm != nextdp->depth.mm) || gaschange_after) {
					if (dp->setpoint) {
						put_format_loc(&buf, translate("gettextFromC", "Stay at %.*f %s for %d:%02d min - runtime %d:%02u on %s (SP = %.1fbar CCR)"),
							     decimals, depthvalue, depth_unit,
							     FRACTION(dp->time - lasttime, 60),
							     FRACTION(dp->time, 60),
							     gasname(gasmix),
							     (double) dp->setpoint / 1000.0);
					} else {
						put_format_loc(&buf, translate("gettextFromC", "Stay at %.*f %s for %d:%02d min - runtime %d:%02u on %s %s"),
							     decimals, depthvalue, depth_unit,
							     FRACTION(dp->time - lasttime, 60),
							     FRACTION(dp->time, 60),
							     gasname(gasmix),
							     translate("gettextFromC", divemode_text_ui[dp->divemode]));
					}
					put_string(&buf, "<br/>\n");
					newdepth = dp->depth.mm;
					lasttime = dp->time;
				}
			}
		} else {
			/* When not displaying the verbatim dive plan, we typically ignore ascents between deco stops,
			 * unless the display transitions option has been selected.  We output a segment if any of the
			 * following conditions are met.
			 * 1) Display transitions is selected
			 * 2) The segment was manually entered
			 * 3) It is the last segment of the dive
			 * 4) The segment is not an ascent, there was a gas change at the start of the segment and the next segment
			 *    is a change in depth (typical deco stop)
			 * 5) There is a gas change at the end of the segment and the last segment was entered (first calculated
			 *    segment if it ends in a gas change)
			 * 6) There is a gaschange after but no ascent.  This should only occur when backgas breaks option is selected
			 * 7) It is an ascent ending with a gas change, but is not followed by a stop.   As case 5 already matches
			 *    the first calculated ascent if it ends with a gas change, this should only occur if a travel gas is
			 *    used for a calculated ascent, there is a subsequent gas change before the first deco stop, and zero
			 *    time has been allowed for a gas switch.
			 */
			if (plan_display_transitions || dp->entered || !dp->next ||
			    (nextdp && dp->depth.mm != nextdp->depth.mm) ||
			    (!isascent && (gaschange_before || rebreatherchange_before) && nextdp && dp->depth.mm != nextdp->depth.mm) ||
			    ((gaschange_after || rebreatherchange_after) && lastentered) || ((gaschange_after || rebreatherchange_after)&& !isascent) ||
			    (isascent && (gaschange_after || rebreatherchange_after) && nextdp && dp->depth.mm != nextdp->depth.mm ) ||
			    (lastentered && !dp->entered && dp->next->depth.mm == dp->depth.mm)) {
				// Print a symbol to indicate whether segment is an ascent, descent, constant depth (user entered) or deco stop
				if (isascent)
					segmentsymbol = "&#10138;"; // up-right arrow for ascent
				else if (dp->depth.mm > lastdepth)
					segmentsymbol = "&#10136;"; // down-right arrow for descent
				else if (dp->entered)
					segmentsymbol = "&#10137;"; // right arrow for entered entered segment at constant depth
				else
					segmentsymbol = "-";        // minus sign (a.k.a. horizontal line) for deco stop

				put_format(&buf, "<tr><td style='padding-left: 10px; float: right;'>%s</td>", segmentsymbol);

				asprintf_loc(&temp, translate("gettextFromC", "%3.0f%s"), depthvalue, depth_unit);
				put_format(&buf, "<td style='padding-left: 10px; float: right;'>%s</td>", temp);
				free(temp);
				if (plan_display_duration) {
					asprintf_loc(&temp, translate("gettextFromC", "%3dmin"), (dp->time - lasttime + 30) / 60);
					put_format(&buf, "<td style='padding-left: 10px; float: right;'>%s</td>", temp);
					free(temp);
				}
				if (plan_display_runtime) {
					asprintf_loc(&temp, translate("gettextFromC", "%3dmin"), (dp->time + 30) / 60);
					put_format(&buf, "<td style='padding-left: 10px; float: right;'>%s</td>", temp);
					free(temp);
				}

				/* Normally a gas change is displayed on the stopping segment, so only display a gas change at the end of
				 * an ascent segment if it is not followed by a stop
				 */
				if ((isascent || dp->entered) && gaschange_after && dp->next && nextdp && (dp->depth.mm != nextdp->depth.mm || nextdp->entered)) {
					if (dp->setpoint) {
						asprintf_loc(&temp, translate("gettextFromC", "(SP = %.1fbar CCR)"), dp->setpoint / 1000.0);
						put_format(&buf, "<td style='padding-left: 10px; color: red; float: left;'><b>%s %s</b></td>",
							gasname(newgasmix), temp);
						free(temp);
					} else {
						put_format(&buf, "<td style='padding-left: 10px; color: red; float: left;'><b>%s %s</b></td>",
							gasname(newgasmix), lastdivemode == UNDEF_COMP_TYPE || lastdivemode == dp->divemode ? "" : translate("gettextFromC", divemode_text_ui[dp->divemode]));
						if (isascent && (get_he(lastprintgasmix) > 0)) { // For a trimix gas change on ascent, save ICD info if previous cylinder had helium
							if (isobaric_counterdiffusion(lastprintgasmix, newgasmix, &icdvalues)) // Do icd calulations
								icdwarning = true;
							if (icdvalues.dN2 > 0) { // If the gas change involved helium as well as an increase in nitrogen..
								add_icd_entry(&icdbuf, &icdvalues, icdtableheader, dp->time, depth_to_mbar(dp->depth.mm, dive), lastprintgasmix, newgasmix); // .. then print calculations to buffer.
								icdtableheader = false;
							}
						}
					}
					lastprintsetpoint = dp->setpoint;
					lastprintgasmix = newgasmix;
					lastdivemode = dp->divemode;
					gaschange_after = false;
				} else if (gaschange_before || rebreatherchange_before) {
					// If a new gas has been used for this segment, now is the time to show it
					if (dp->setpoint) {
						asprintf_loc(&temp, translate("gettextFromC", "(SP = %.1fbar CCR)"), (double) dp->setpoint / 1000.0);
						put_format(&buf, "<td style='padding-left: 10px; color: red; float: left;'><b>%s %s</b></td>", gasname(gasmix), temp);
						free(temp);
					} else {
						put_format(&buf, "<td style='padding-left: 10px; color: red; float: left;'><b>%s %s</b></td>", gasname(gasmix),
							   lastdivemode == UNDEF_COMP_TYPE || lastdivemode == dp->divemode ? "" : translate("gettextFromC", divemode_text_ui[dp->divemode]));
						if (get_he(lastprintgasmix) > 0) {  // For a trimix gas change, save ICD info if previous cylinder had helium
							if (isobaric_counterdiffusion(lastprintgasmix, gasmix, &icdvalues))  // Do icd calculations
								icdwarning = true;
							if (icdvalues.dN2 > 0) { // If the gas change involved helium as well as an increase in nitrogen..
								add_icd_entry(&icdbuf, &icdvalues, icdtableheader, lasttime, depth_to_mbar(dp->depth.mm, dive), lastprintgasmix, gasmix); // .. then print data to buffer.
								icdtableheader = false;
							}
						}
					}
					// Set variables so subsequent iterations can test against the last gas printed
					lastprintsetpoint = dp->setpoint;
					lastprintgasmix = gasmix;
					lastdivemode = dp->divemode;
					gaschange_after = false;
				} else {
					put_string(&buf, "<td>&nbsp;</td>");
				}
				put_string(&buf, "</tr>\n");
				newdepth = dp->depth.mm;
				// Only add the time we actually displayed so rounding errors dont accumulate
				lasttime += ((dp->time - lasttime + 30) / 60) * 60;
			}
		}
		if (gaschange_after || gaschange_before) {
			// gas switch at this waypoint for verbatim
			if (plan_verbatim) {
				if (lastsetpoint >= 0) {
					if (nextdp && nextdp->setpoint) {
						put_format_loc(&buf, translate("gettextFromC", "Switch gas to %s (SP = %.1fbar)"), gasname(newgasmix), (double) nextdp->setpoint / 1000.0);
					} else {
						put_format(&buf, translate("gettextFromC", "Switch gas to %s"), gasname(newgasmix));
						if ((isascent) && (get_he(lastprintgasmix) > 0)) {          // For a trimix gas change on ascent:
							if (isobaric_counterdiffusion(lastprintgasmix, newgasmix, &icdvalues)) // Do icd calculations
								icdwarning = true;
							if (icdvalues.dN2 > 0) { // If the gas change involved helium as well as an increase in nitrogen..
								add_icd_entry(&icdbuf, &icdvalues, icdtableheader, dp->time, depth_to_mbar(dp->depth.mm, dive), lastprintgasmix, newgasmix); // ... then print data to buffer.
								icdtableheader = false;
							}
						}
					}
					put_string(&buf, "<br/>\n");
				}
				lastprintgasmix = newgasmix;
				gaschange_after = false;
				gasmix = newgasmix;
			}
		}
		lastprintdepth = newdepth;
		lastdepth = dp->depth.mm;
		lastsetpoint = dp->setpoint;
		lastentered = dp->entered;
	} while ((dp = nextdp) != NULL);
	if (!plan_verbatim)
		put_string(&buf, "</tbody>\n</table>\n<br/>\n");

	/* Print the CNS and OTU next.*/
	dive->cns = 0;
	dive->maxcns = 0;
	update_cylinder_related_info(dive);
	put_format_loc(&buf, "<div>\n%s: %i%%", translate("gettextFromC", "CNS"), dive->cns);
	put_format_loc(&buf, "<br/>\n%s: %i<br/>\n</div>\n", translate("gettextFromC", "OTU"), dive->otu);

	/* Print the settings for the diveplan next. */
	put_string(&buf, "<div>\n");
	if (decoMode(true) == BUEHLMANN) {
		put_format_loc(&buf, translate("gettextFromC", "Deco model: Bühlmann ZHL-16C with GFLow = %d%% and GFHigh = %d%%"), diveplan->gflow, diveplan->gfhigh);
	} else if (decoMode(true) == VPMB) {
		if (diveplan->vpmb_conservatism == 0)
			put_string(&buf, translate("gettextFromC", "Deco model: VPM-B at nominal conservatism"));
		else
			put_format_loc(&buf, translate("gettextFromC", "Deco model: VPM-B at +%d conservatism"), diveplan->vpmb_conservatism);
		if (diveplan->eff_gflow)
			put_format_loc(&buf,  translate("gettextFromC", ", effective GF=%d/%d"), diveplan->eff_gflow, diveplan->eff_gfhigh);
	} else if (decoMode(true) == RECREATIONAL) {
		put_format_loc(&buf, translate("gettextFromC", "Deco model: Recreational mode based on Bühlmann ZHL-16B with GFLow = %d%% and GFHigh = %d%%"),
			     diveplan->gflow, diveplan->gfhigh);
	}
	put_string(&buf, "<br/>\n");

	const char *depth_unit;
	int altitude = (int) get_depth_units((int) (pressure_to_altitude(diveplan->surface_pressure)), NULL, &depth_unit);

	put_format_loc(&buf, translate("gettextFromC", "ATM pressure: %dmbar (%d%s)<br/>\n</div>\n"), diveplan->surface_pressure, altitude, depth_unit);

	/* Get SAC values and units for printing it in gas consumption */
	double bottomsacvalue, decosacvalue;
	int sacdecimals;
	const char* sacunit;

	bottomsacvalue = get_volume_units(prefs.bottomsac, &sacdecimals, &sacunit);
	decosacvalue = get_volume_units(prefs.decosac, NULL, NULL);

	/* Reduce number of decimals from 1 to 0 for bar/min, keep 2 for cuft/min */
	if (sacdecimals==1) sacdecimals--;

	/* Print the gas consumption next.*/
	if (dive->dc.divemode == CCR)
		temp = strdup(translate("gettextFromC", "Gas consumption (CCR legs excluded):"));
	else
		asprintf_loc(&temp, "%s %.*f|%.*f%s/min):", translate("gettextFromC", "Gas consumption (based on SAC"),
			     sacdecimals, bottomsacvalue, sacdecimals, decosacvalue, sacunit);
	put_format(&buf, "<div>\n%s<br/>\n", temp);
	free(temp);

	/* Print gas consumption: This loop covers all cylinders */
	for (int gasidx = 0; gasidx < dive->cylinders.nr; gasidx++) {
		double volume, pressure, deco_volume, deco_pressure, mingas_volume, mingas_pressure, mingas_d_pressure, mingas_depth;
		const char *unit, *pressure_unit, *depth_unit;
		char warning[1000] = "";
		char mingas[1000] = "";
		cylinder_t *cyl = get_cylinder(dive, gasidx);
		if (cyl->cylinder_use == NOT_USED)
			continue;

		volume = get_volume_units(cyl->gas_used.mliter, NULL, &unit);
		deco_volume = get_volume_units(cyl->deco_gas_used.mliter, NULL, &unit);
		if (cyl->type.size.mliter) {
			int remaining_gas = lrint((double)cyl->end.mbar * cyl->type.size.mliter / 1000.0 / gas_compressibility_factor(cyl->gasmix, cyl->end.mbar / 1000.0));
			double deco_pressure_mbar = isothermal_pressure(cyl->gasmix, 1.0, remaining_gas + cyl->deco_gas_used.mliter,
				cyl->type.size.mliter) * 1000 - cyl->end.mbar;
			deco_pressure = get_pressure_units(lrint(deco_pressure_mbar), &pressure_unit);
			pressure = get_pressure_units(cyl->start.mbar - cyl->end.mbar, &pressure_unit);
			/* Warn if the plan uses more gas than is available in a cylinder
			 * This only works if we have working pressure for the cylinder
			 * 10bar is a made up number - but it seemed silly to pretend you could breathe cylinder down to 0 */
			if (cyl->end.mbar < 10000)
				snprintf(warning, sizeof(warning), "<br/>\n&nbsp;&mdash; <span style='color: red;'>%s </span> %s",
					translate("gettextFromC", "Warning:"),
					translate("gettextFromC", "this is more gas than available in the specified cylinder!"));
			else
				if (cyl->end.mbar / 1000.0 * cyl->type.size.mliter / gas_compressibility_factor(cyl->gasmix, cyl->end.mbar / 1000.0)
				    < cyl->deco_gas_used.mliter)
					snprintf(warning, sizeof(warning), "<br/>\n&nbsp;&mdash; <span style='color: red;'>%s </span> %s",
						translate("gettextFromC", "Warning:"),
						translate("gettextFromC", "not enough reserve for gas sharing on ascent!"));

			/* Do and print minimum gas calculation for last bottom gas, but only for OC mode, */
			/* not for recreational mode and if no other warning was set before. */
			else
				if (lastbottomdp && gasidx == lastbottomdp->cylinderid
					&& dive->dc.divemode == OC && decoMode(true) != RECREATIONAL) {
					/* Calculate minimum gas volume. */
					volume_t mingasv;
					mingasv.mliter = lrint(prefs.sacfactor / 100.0 * prefs.problemsolvingtime * prefs.bottomsac
						* depth_to_bar(lastbottomdp->depth.mm, dive)
						+ prefs.sacfactor / 100.0 * cyl->deco_gas_used.mliter);
					/* Calculate minimum gas pressure for cyclinder. */
					lastbottomdp->minimum_gas.mbar = lrint(isothermal_pressure(cyl->gasmix, 1.0,
						mingasv.mliter, cyl->type.size.mliter) * 1000);
					/* Translate all results into correct units */
					mingas_volume = get_volume_units(mingasv.mliter, NULL, &unit);
					mingas_pressure = get_pressure_units(lastbottomdp->minimum_gas.mbar, &pressure_unit);
					mingas_d_pressure = get_pressure_units(lrint((double)cyl->end.mbar + deco_pressure_mbar - lastbottomdp->minimum_gas.mbar), &pressure_unit);
					mingas_depth = get_depth_units(lastbottomdp->depth.mm, NULL, &depth_unit);
					/* Print it to results */
					if (cyl->start.mbar > lastbottomdp->minimum_gas.mbar) {
						snprintf_loc(mingas, sizeof(mingas), "<br/>\n&nbsp;&mdash; <span style='color: %s;'>%s</span> (%s %.1fx%s/+%d%s@%.0f%s): "
							     "%.0f%s/%.0f%s<span style='color: %s;'>/&Delta;:%+.0f%s</span>",
							     mingas_d_pressure > 0 ? "green" :"red",
							     translate("gettextFromC", "Minimum gas"),
							     translate("gettextFromC", "based on"),
							     prefs.sacfactor / 100.0,
							     translate("gettextFromC", "SAC"),
							     prefs.problemsolvingtime,
							     translate("gettextFromC", "min"),
							     mingas_depth, depth_unit,
							     mingas_volume, unit,
							     mingas_pressure, pressure_unit,
							     mingas_d_pressure > 0 ? "grey" :"indianred",
							     mingas_d_pressure, pressure_unit);
					} else {
						snprintf(warning, sizeof(warning), "<br/>\n&nbsp;&mdash; <span style='color: red;'>%s </span> %s",
							translate("gettextFromC", "Warning:"),
							translate("gettextFromC", "required minimum gas for ascent already exceeding start pressure of cylinder!"));
					}
				}
			/* Print the gas consumption for every cylinder here to temp buffer. */
			if (lrint(volume) > 0) {
				asprintf_loc(&temp, translate("gettextFromC", "%.0f%s/%.0f%s of <span style='color: red;'><b>%s</b></span> (%.0f%s/%.0f%s in planned ascent)"),
					     volume, unit, pressure, pressure_unit, gasname(cyl->gasmix), deco_volume, unit, deco_pressure, pressure_unit);
			} else {
				asprintf_loc(&temp, translate("gettextFromC", "%.0f%s/%.0f%s of <span style='color: red;'><b>%s</b></span>"),
					     volume, unit, pressure, pressure_unit, gasname(cyl->gasmix));
			}
		} else {
			if (lrint(volume) > 0) {
				asprintf_loc(&temp, translate("gettextFromC", "%.0f%s of <span style='color: red;'><b>%s</b></span> (%.0f%s during planned ascent)"),
					     volume, unit, gasname(cyl->gasmix), deco_volume, unit);
			} else {
				asprintf_loc(&temp, translate("gettextFromC", "%.0f%s of <span style='color: red;'><b>%s</b></span>"),
					     volume, unit, gasname(cyl->gasmix));
			}
		}
		/* Gas consumption: Now finally print all strings to output */
		put_format(&buf, "%s%s%s<br/>\n", temp, warning, mingas);
		free(temp);
	}
	put_format(&buf, "</div>\n");

	/* For trimix OC dives, if an icd table header and icd data were printed to buffer, then add the ICD table here */
	if (!icdtableheader && prefs.show_icd) {
		put_string(&icdbuf, "</tbody></table>\n"); // End the ICD table
		mb_cstring(&icdbuf);
		put_string(&buf, icdbuf.buffer); // ..and add it to the html buffer
		if (icdwarning) { // If necessary, add warning
			put_format(&buf, "<span style='color: red;'>%s</span> %s",
				translate("gettextFromC", "Warning:"),
				translate("gettextFromC", "Isobaric counterdiffusion conditions exceeded"));
		}
		put_string(&buf, "<br/></div>\n");
	}
	free_buffer(&icdbuf);

	/* Print warnings for pO2 */
	dp = diveplan->dp;
	bool o2warning_exist = false;
	enum divemode_t current_divemode;
	double amb;
	const struct event *evd = NULL;
	current_divemode = UNDEF_COMP_TYPE;

	if (dive->dc.divemode != CCR) {
		while (dp) {
			if (dp->time != 0) {
				struct gas_pressures pressures;
				struct gasmix gasmix = get_cylinder(dive, dp->cylinderid)->gasmix;

				current_divemode = get_current_divemode(&dive->dc, dp->time, &evd, &current_divemode);
				amb = depth_to_atm(dp->depth.mm, dive);
				fill_pressures(&pressures, amb, gasmix, (current_divemode == OC) ? 0.0 : amb * gasmix.o2.permille / 1000.0, current_divemode);

				if (pressures.o2 > (dp->entered ? prefs.bottompo2 : prefs.decopo2) / 1000.0) {
					const char *depth_unit;
					int decimals;
					double depth_value = get_depth_units(dp->depth.mm, &decimals, &depth_unit);
					if (!o2warning_exist)
						put_string(&buf, "<div>\n");
					o2warning_exist = true;
					asprintf_loc(&temp, translate("gettextFromC", "high pO₂ value %.2f at %d:%02u with gas %s at depth %.*f %s"),
						pressures.o2, FRACTION(dp->time, 60), gasname(gasmix), decimals, depth_value, depth_unit);
					put_format(&buf, "<span style='color: red;'>%s </span> %s<br/>\n", translate("gettextFromC", "Warning:"), temp);
					free(temp);
				} else if (pressures.o2 < 0.16) {
					const char *depth_unit;
					int decimals;
					double depth_value = get_depth_units(dp->depth.mm, &decimals, &depth_unit);
					if (!o2warning_exist)
						put_string(&buf, "<div>");
					o2warning_exist = true;
					asprintf_loc(&temp, translate("gettextFromC", "low pO₂ value %.2f at %d:%02u with gas %s at depth %.*f %s"),
						pressures.o2, FRACTION(dp->time, 60), gasname(gasmix), decimals, depth_value, depth_unit);
					put_format(&buf, "<span style='color: red;'>%s </span> %s<br/>\n", translate("gettextFromC", "Warning:"), temp);
					free(temp);
				}
			}
			dp = dp->next;
		}
	}
	if (o2warning_exist)
		put_string(&buf, "</div>\n");
finished:
	free(dive->notes);
	dive->notes = detach_cstring(&buf);
#ifdef DEBUG_PLANNER_NOTES
	printf("<!DOCTYPE html>\n<html>\n\t<head><title>plannernotes</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/></head>\n\t<body>\n%s\t</body>\n</html>\n", dive->notes);
#endif
}
