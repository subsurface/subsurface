// SPDX-License-Identifier: GPL-2.0
/* plannernotes.cpp
 *
 * format notes describing a dive plan
 *
 * (c) Dirk Hohndel 2013
 */
#include <assert.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include "deco.h"
#include "dive.h"
#include "divelist.h"
#include "divelog.h"
#include "event.h"
#include "units.h"
#include "planner.h"
#include "range.h"
#include "gettext.h"
#include "libdivecomputer/parser.h"
#include "qthelper.h"
#include "format.h"
#include "subsurface-string.h"
#include "version.h"

int diveplan::duration() const
{
	int duration = 0;
	int lastdepth = 0;
	for (auto &dp: this->dp) {
		if (dp.time > duration && (dp.depth.mm > SURFACE_THRESHOLD || lastdepth > SURFACE_THRESHOLD)) {
			duration = dp.time;
			lastdepth = dp.depth.mm;
		}
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
static std::string icd_entry(struct icd_data *icdvalues, bool printheader, int time_seconds, int ambientpressure_mbar, struct gasmix gas_from, struct gasmix gas_to)
{
	std::string b;
	if (printheader) { // Create a table description and a table header if no icd data have been written yet.
		b += format_string_std("<div>%s:", translate("gettextFromC","Isobaric counterdiffusion information"));
		b += format_string_std("<table><tr><td align='left'><b>%s</b></td>", translate("gettextFromC", "runtime"));
		b += format_string_std("<td align='center'><b>%s</b></td>", translate("gettextFromC", "gaschange"));
		b += format_string_std("<td style='padding-left: 15px;'><b>%s</b></td>", translate("gettextFromC", "&#916;He"));
		b += format_string_std("<td style='padding-left: 20px;'><b>%s</b></td>", translate("gettextFromC", "&#916;N&#8322;"));
		b += format_string_std("<td style='padding-left: 10px;'><b>%s</b></td></tr>", translate("gettextFromC", "max &#916;N&#8322;"));
	}		// Add one entry to the icd table:
	b += casprintf_loc(
		"<tr><td rowspan='2' style= 'vertical-align:top;'>%3d%s</td>"
		"<td rowspan=2 style= 'vertical-align:top;'>%s&#10137;",
		(time_seconds + 30) / 60, translate("gettextFromC", "min"), gas_from.name().c_str());
	b += casprintf_loc(
		"%s</td><td style='padding-left: 10px;'>%+5.1f%%</td>"
		"<td style= 'padding-left: 15px; color:%s;'>%+5.1f%%</td>"
		"<td style='padding-left: 15px;'>%+5.1f%%</td></tr>"
		"<tr><td style='padding-left: 10px;'>%+5.2f%s</td>"
		"<td style='padding-left: 15px; color:%s;'>%+5.2f%s</td>"
		"<td style='padding-left: 15px;'>%+5.2f%s</td></tr>",
		gas_to.name().c_str(), icdvalues->dHe / 10.0,
		((5 * icdvalues->dN2) > -icdvalues->dHe) ? "red" : "#383838", icdvalues->dN2 / 10.0 , 0.2 * (-icdvalues->dHe / 10.0),
		ambientpressure_mbar * icdvalues->dHe / 1e6f, translate("gettextFromC", "bar"), ((5 * icdvalues->dN2) > -icdvalues->dHe) ? "red" : "#383838",
		ambientpressure_mbar * icdvalues->dN2 / 1e6f, translate("gettextFromC", "bar"),
		ambientpressure_mbar * -icdvalues->dHe / 5e6f, translate("gettextFromC", "bar"));
	return b;
}

const char *get_planner_disclaimer()
{
	return translate("gettextFromC", "DISCLAIMER / WARNING: THIS IMPLEMENTATION OF THE %s "
			 "ALGORITHM AND A DIVE PLANNER IMPLEMENTATION BASED ON THAT HAS "
			 "RECEIVED ONLY A LIMITED AMOUNT OF TESTING. WE STRONGLY RECOMMEND NOT TO "
			 "PLAN DIVES SIMPLY BASED ON THE RESULTS GIVEN HERE.");
}

/* Returns newly allocated buffer. Must be freed by caller */
extern std::string get_planner_disclaimer_formatted()
{
	const char *deco = decoMode(true) == VPMB ? translate("gettextFromC", "VPM-B")
						  : translate("gettextFromC", "BUHLMANN");
	return format_string_std(get_planner_disclaimer(), deco);
}

void diveplan::add_plan_to_notes(struct dive &dive, bool show_disclaimer, planner_error_t error)
{
	std::string buf;
	std::string icdbuf;
	const char *segmentsymbol;
	int lastdepth = 0, lasttime = 0, lastsetpoint = -1, newdepth = 0, lastprintdepth = 0, lastprintsetpoint = -1;
	struct gasmix lastprintgasmix = gasmix_invalid;
	bool plan_verbatim = prefs.verbatim_plan;
	bool plan_display_runtime = prefs.display_runtime;
	bool plan_display_duration = prefs.display_duration;
	bool plan_display_transitions = prefs.display_transitions;
	bool rebreatherchange_after = !plan_verbatim;
	bool rebreatherchange_before;
	enum divemode_t lastdivemode = UNDEF_COMP_TYPE;
	bool lastentered = true;
	bool icdwarning = false, icdtableheader = true;
	struct divedatapoint *lastbottomdp = nullptr;
	struct icd_data icdvalues;

	if (dp.empty())
		return;

	if (error != PLAN_OK) {
		const char *message;
		switch (error) {
		case PLAN_ERROR_TIMEOUT:
			message = translate("gettextFromC", "Decompression calculation aborted due to excessive time");

			break;
		case PLAN_ERROR_INAPPROPRIATE_GAS:
			message = translate("gettextFromC", "One or more tanks with a tank use type inappropriate for the selected dive mode are included in the dive plan. "
				"Please change them to appropriate tanks to enable the generation of a dive plan.");

			break;
		default:
			message = translate("gettextFromC", "An error occurred during dive plan generation");

			break;
		}

		buf += format_string_std("<span style='color: red;'>%s </span> %s<br/>",
			translate("gettextFromC", "Warning:"), message);

		dive.notes = std::move(buf);

		return;
	}

	if (show_disclaimer) {
		buf += "<div><b>";
		buf += get_planner_disclaimer_formatted();
		buf += "</b><br/>\n</div>\n";
	}

	buf += "<div>\n<b>";
	if (surface_interval < 0) {
		buf += format_string_std("%s (%s) %s",
			translate("gettextFromC", "Subsurface"),
			subsurface_canonical_version(),
			translate("gettextFromC", "dive plan</b> (overlapping dives detected)"));
		dive.notes = std::move(buf);
		return;
	} else if (surface_interval >= 48 * 60 *60) {
		buf += format_string_std("%s (%s) %s %s",
			translate("gettextFromC", "Subsurface"),
			subsurface_canonical_version(),
			translate("gettextFromC", "dive plan</b> created on"),
			get_current_date().c_str());
	} else {
		buf += casprintf_loc("%s (%s) %s %d:%02d) %s %s",
			translate("gettextFromC", "Subsurface"),
			subsurface_canonical_version(),
			translate("gettextFromC", "dive plan</b> (surface interval "),
			FRACTION_TUPLE(surface_interval / 60, 60),
			translate("gettextFromC", "created on"),
			get_current_date().c_str());
	}
	buf += "<br/>\n";

	if (prefs.display_variations && decoMode(true) != RECREATIONAL)
		buf += casprintf_loc(translate("gettextFromC", "Runtime: %dmin%s"),
			duration(), "VARIATIONS");
	else
		buf += casprintf_loc(translate("gettextFromC", "Runtime: %dmin%s"),
			duration(), "");
	buf += "<br/>\n</div>\n";

	if (!plan_verbatim) {
		buf += format_string_std("<table>\n<thead>\n<tr><th></th><th>%s</th>", translate("gettextFromC", "depth"));
		if (plan_display_duration)
			buf += format_string_std("<th style='padding-left: 10px;'>%s</th>", translate("gettextFromC", "duration"));
		if (plan_display_runtime)
			buf += format_string_std("<th style='padding-left: 10px;'>%s</th>", translate("gettextFromC", "runtime"));
		buf += format_string_std("<th style='padding-left: 10px; float: left;'>%s</th></tr>\n</thead>\n<tbody style='float: left;'>\n",
				translate("gettextFromC", "gas"));
	}

	for (auto dp = this->dp.begin(); dp != this->dp.end(); ++dp) {
		auto nextdp = std::next(dp);
		struct gasmix gasmix, newgasmix = {};
		const char *depth_unit;
		double depthvalue;
		int decimals;
		bool isascent = (dp->depth.mm < lastdepth);

		if (dp->time == 0)
			continue;
		gasmix = dive.get_cylinder(dp->cylinderid)->gasmix;
		depthvalue = get_depth_units(dp->depth, &decimals, &depth_unit);
		/* analyze the dive points ahead */
		while (nextdp != this->dp.end() && nextdp->time == 0)
			++nextdp;
		bool atend = nextdp == this->dp.end();
		if (!atend)
			newgasmix = dive.get_cylinder(nextdp->cylinderid)->gasmix;
		bool gaschange_after = (!atend && (gasmix_distance(gasmix, newgasmix)));
		bool gaschange_before =  (gasmix_distance(lastprintgasmix, gasmix));
		rebreatherchange_after = (!atend && (dp->setpoint != nextdp->setpoint || dp->divemode != nextdp->divemode));
		rebreatherchange_before = lastprintsetpoint != dp->setpoint || lastdivemode != dp->divemode;
		/* do we want to skip this leg as it is devoid of anything useful? */
		if (!dp->entered &&
		    !atend &&
		    dp->depth.mm != lastdepth &&
		    nextdp->depth.mm != dp->depth.mm &&
		    !gaschange_before &&
		    !gaschange_after &&
		    !rebreatherchange_before &&
		    !rebreatherchange_after)
			continue;
		// Ignore final surface segment for notes
		if (lastdepth == 0 && dp->depth.mm == 0 && atend)
			continue;
		if ((dp->time - lasttime < 10 && lastdepth == dp->depth.mm) && !(gaschange_after && !atend && dp->depth.mm != nextdp->depth.mm))
			continue;

		/* Store pointer to last entered datapoint for minimum gas calculation */
		if (dp->entered && !atend && !nextdp->entered)
			lastbottomdp = &*dp;

		if (plan_verbatim) {
			/* When displaying a verbatim plan, we output a waypoint for every gas change.
			 * Therefore, we do not need to test for difficult cases that mean we need to
			 * print a segment just so we don't miss a gas change.  This makes the logic
			 * to determine whether or not to print a segment much simpler than  with the
			 * non-verbatim plan.
			 */
			if (dp->depth.mm != lastprintdepth) {
				if (plan_display_transitions || dp->entered || atend || (gaschange_after && !atend && dp->depth.mm != nextdp->depth.mm)) {
					if (dp->setpoint) {
						buf += casprintf_loc(translate("gettextFromC", "%s to %.*f %s in %d:%02d min - runtime %d:%02u on %s (SP = %.1fbar)"),
							     dp->depth.mm < lastprintdepth ? translate("gettextFromC", "Ascend") : translate("gettextFromC", "Descend"),
							     decimals, depthvalue, depth_unit,
							     FRACTION_TUPLE(dp->time - lasttime, 60),
							     FRACTION_TUPLE(dp->time, 60),
							     gasmix.name().c_str(),
							     (double) dp->setpoint / 1000.0);
					} else {
						buf += casprintf_loc(translate("gettextFromC", "%s to %.*f %s in %d:%02d min - runtime %d:%02u on %s"),
							     dp->depth.mm < lastprintdepth ? translate("gettextFromC", "Ascend") : translate("gettextFromC", "Descend"),
							     decimals, depthvalue, depth_unit,
							     FRACTION_TUPLE(dp->time - lasttime, 60),
							     FRACTION_TUPLE(dp->time, 60),
							     gasmix.name().c_str());
					}

					buf += "<br/>\n";
				}
				newdepth = dp->depth.mm;
				lasttime = dp->time;
			} else {
				if ((!atend && dp->depth.mm != nextdp->depth.mm) || gaschange_after) {
					if (dp->setpoint) {
						buf += casprintf_loc(translate("gettextFromC", "Stay at %.*f %s for %d:%02d min - runtime %d:%02u on %s (SP = %.1fbar CCR)"),
							     decimals, depthvalue, depth_unit,
							     FRACTION_TUPLE(dp->time - lasttime, 60),
							     FRACTION_TUPLE(dp->time, 60),
							     gasmix.name().c_str(),
							     (double) dp->setpoint / 1000.0);
					} else {
						buf += casprintf_loc(translate("gettextFromC", "Stay at %.*f %s for %d:%02d min - runtime %d:%02u on %s %s"),
							     decimals, depthvalue, depth_unit,
							     FRACTION_TUPLE(dp->time - lasttime, 60),
							     FRACTION_TUPLE(dp->time, 60),
							     gasmix.name().c_str(),
							     translate("gettextFromC", divemode_text_ui[dp->divemode]));
					}
					buf += "<br/>\n";
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
			if (plan_display_transitions || dp->entered || atend ||
			    (!atend && dp->depth.mm != nextdp->depth.mm) ||
			    (!isascent && (gaschange_before || rebreatherchange_before) && !atend && dp->depth.mm != nextdp->depth.mm) ||
			    ((gaschange_after || rebreatherchange_after) && lastentered) || ((gaschange_after || rebreatherchange_after)&& !isascent) ||
			    (isascent && (gaschange_after || rebreatherchange_after) && !atend && dp->depth.mm != nextdp->depth.mm ) ||
			    (lastentered && !dp->entered && nextdp->depth.mm == dp->depth.mm)) {
				// Print a symbol to indicate whether segment is an ascent, descent, constant depth (user entered) or deco stop
				if (isascent)
					segmentsymbol = "&#10138;"; // up-right arrow for ascent
				else if (dp->depth.mm > lastdepth)
					segmentsymbol = "&#10136;"; // down-right arrow for descent
				else if (dp->entered)
					segmentsymbol = "&#10137;"; // right arrow for entered entered segment at constant depth
				else
					segmentsymbol = "-";        // minus sign (a.k.a. horizontal line) for deco stop

				buf += format_string_std("<tr><td style='padding-left: 10px; float: right;'>%s</td>", segmentsymbol);

				std::string temp = casprintf_loc(translate("gettextFromC", "%3.0f%s"), depthvalue, depth_unit);
				buf += format_string_std("<td style='padding-left: 10px; float: right;'>%s</td>", temp.c_str());
				if (plan_display_duration) {
					temp = casprintf_loc(translate("gettextFromC", "%3dmin"), (dp->time - lasttime + 30) / 60);
					buf += format_string_std("<td style='padding-left: 10px; float: right;'>%s</td>", temp.c_str());
				}
				if (plan_display_runtime) {
					temp = casprintf_loc(translate("gettextFromC", "%3dmin"), (dp->time + 30) / 60);
					buf += format_string_std("<td style='padding-left: 10px; float: right;'>%s</td>", temp.c_str());
				}

				/* Normally a gas change is displayed on the stopping segment, so only display a gas change at the end of
				 * an ascent segment if it is not followed by a stop
				 */
				if (isascent && gaschange_after && !atend && nextdp->entered) {
					if (nextdp->setpoint) {
						temp = casprintf_loc(translate("gettextFromC", "(SP = %.1fbar CCR)"), nextdp->setpoint / 1000.0);
						buf += format_string_std("<td style='padding-left: 10px; color: red; float: left;'><b>%s %s</b></td>",
							newgasmix.name().c_str(), temp.c_str());
					} else {
						buf += format_string_std("<td style='padding-left: 10px; color: red; float: left;'><b>%s %s</b></td>",
							newgasmix.name().c_str(), dp->divemode == UNDEF_COMP_TYPE || dp->divemode == nextdp->divemode ? "" : translate("gettextFromC", divemode_text_ui[nextdp->divemode]));
						if (isascent && (get_he(lastprintgasmix) > 0)) { // For a trimix gas change on ascent, save ICD info if previous cylinder had helium
							if (isobaric_counterdiffusion(lastprintgasmix, newgasmix, &icdvalues)) // Do icd calulations
								icdwarning = true;
							if (icdvalues.dN2 > 0) { // If the gas change involved helium as well as an increase in nitrogen..
								icdbuf += icd_entry(&icdvalues, icdtableheader, dp->time, dive.depth_to_mbar(dp->depth), lastprintgasmix, newgasmix); // .. then print calculations to buffer.
								icdtableheader = false;
							}
						}
					}
					lastprintsetpoint = nextdp->setpoint;
					lastprintgasmix = newgasmix;
					lastdivemode = nextdp->divemode;
					gaschange_after = false;
				} else if (gaschange_before || rebreatherchange_before) {
					// If a new gas has been used for this segment, now is the time to show it
					if (dp->setpoint) {
						temp = casprintf_loc(translate("gettextFromC", "(SP = %.1fbar CCR)"), (double) dp->setpoint / 1000.0);
						buf += format_string_std("<td style='padding-left: 10px; color: red; float: left;'><b>%s %s</b></td>", gasmix.name().c_str(), temp.c_str());
					} else {
						buf += format_string_std("<td style='padding-left: 10px; color: red; float: left;'><b>%s %s</b></td>", gasmix.name().c_str(),
							   lastdivemode == UNDEF_COMP_TYPE || lastdivemode == dp->divemode ? "" : translate("gettextFromC", divemode_text_ui[dp->divemode]));
						if (get_he(lastprintgasmix) > 0) {  // For a trimix gas change, save ICD info if previous cylinder had helium
							if (isobaric_counterdiffusion(lastprintgasmix, gasmix, &icdvalues))  // Do icd calculations
								icdwarning = true;
							if (icdvalues.dN2 > 0) { // If the gas change involved helium as well as an increase in nitrogen..
								icdbuf += icd_entry(&icdvalues, icdtableheader, lasttime, dive.depth_to_mbar(dp->depth), lastprintgasmix, gasmix); // .. then print data to buffer.
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
					buf += "<td>&nbsp;</td>";
				}
				buf += "</tr>\n";
				newdepth = dp->depth.mm;
				// Only add the time we actually displayed so rounding errors dont accumulate
				lasttime += ((dp->time - lasttime + 30) / 60) * 60;
			}
		}
		if (gaschange_after || gaschange_before) {
			// gas switch at this waypoint for verbatim
			if (plan_verbatim) {
				if (lastsetpoint >= 0) {
					if (!atend && nextdp->setpoint) {
						buf += casprintf_loc(translate("gettextFromC", "Switch gas to %s (SP = %.1fbar)"), newgasmix.name().c_str(), (double) nextdp->setpoint / 1000.0);
					} else {
						buf += format_string_std(translate("gettextFromC", "Switch gas to %s"), newgasmix.name().c_str());
						if ((isascent) && (get_he(lastprintgasmix) > 0)) {          // For a trimix gas change on ascent:
							if (isobaric_counterdiffusion(lastprintgasmix, newgasmix, &icdvalues)) // Do icd calculations
								icdwarning = true;
							if (icdvalues.dN2 > 0) { // If the gas change involved helium as well as an increase in nitrogen..
								icdbuf += icd_entry(&icdvalues, icdtableheader, dp->time, dive.depth_to_mbar(dp->depth), lastprintgasmix, newgasmix); // ... then print data to buffer.
								icdtableheader = false;
							}
						}
					}
					buf += "<br/>\n";
				}
				lastprintgasmix = newgasmix;
				gasmix = newgasmix;
			}
		}
		lastprintdepth = newdepth;
		lastdepth = dp->depth.mm;
		lastsetpoint = dp->setpoint;
		lastentered = dp->entered;
	}
	if (!plan_verbatim)
		buf += "</tbody>\n</table>\n<br/>\n";

	/* Print the CNS and OTU next.*/
	dive.cns = 0;
	dive.maxcns = 0;
	divelog.dives.update_cylinder_related_info(dive);
	buf += casprintf_loc("<div>\n%s: %i%%", translate("gettextFromC", "CNS"), dive.cns);
	buf += casprintf_loc("<br/>\n%s: %i<br/>\n</div>\n", translate("gettextFromC", "OTU"), dive.otu);

	/* Print the settings for the diveplan next. */
	buf += "<div>\n";
	if (decoMode(true) == BUEHLMANN) {
		buf += casprintf_loc(translate("gettextFromC", "Deco model: Bühlmann ZHL-16C with GFLow = %d%% and GFHigh = %d%%"), gflow, gfhigh);
	} else if (decoMode(true) == VPMB) {
		if (vpmb_conservatism == 0)
			buf += translate("gettextFromC", "Deco model: VPM-B at nominal conservatism");
		else
			buf += casprintf_loc(translate("gettextFromC", "Deco model: VPM-B at +%d conservatism"), vpmb_conservatism);
		if (eff_gflow)
			buf += casprintf_loc( translate("gettextFromC", ", effective GF=%d/%d"), eff_gflow, eff_gfhigh);
	} else if (decoMode(true) == RECREATIONAL) {
		buf += casprintf_loc(translate("gettextFromC", "Deco model: Recreational mode based on Bühlmann ZHL-16B with GFLow = %d%% and GFHigh = %d%%"),
			     gflow, gfhigh);
	}
	buf += "<br/>\n";

	{
		const char *depth_unit;
		int altitude = (int) get_depth_units(pressure_to_altitude(surface_pressure), NULL, &depth_unit);

		buf += casprintf_loc(translate("gettextFromC", "ATM pressure: %dmbar (%d%s)<br/>\n</div>\n"), surface_pressure, altitude, depth_unit);
	}

	/* Get SAC values and units for printing it in gas consumption */
	{
		double bottomsacvalue, decosacvalue;
		int sacdecimals;
		const char* sacunit;

		bottomsacvalue = get_volume_units(prefs.bottomsac, &sacdecimals, &sacunit);
		decosacvalue = get_volume_units(prefs.decosac, NULL, NULL);

		/* Reduce number of decimals from 1 to 0 for bar/min, keep 2 for cuft/min */
		if (sacdecimals==1) sacdecimals--;

		/* Print the gas consumption next.*/
		std::string temp;
		if (dive.dcs[0].divemode == CCR)
			temp = translate("gettextFromC", "Gas consumption (CCR legs excluded):");
		else
			temp = casprintf_loc("%s %.*f|%.*f%s/min):", translate("gettextFromC", "Gas consumption (based on SAC"),
				     sacdecimals, bottomsacvalue, sacdecimals, decosacvalue, sacunit);
		buf += format_string_std("<div>\n%s<br/>\n", temp.c_str());
	}

	/* Print gas consumption: This loop covers all cylinders */
	for (auto [gasidx, cyl]: enumerated_range(dive.cylinders)) {
		double volume, pressure, deco_volume, deco_pressure, mingas_volume, mingas_pressure, mingas_d_pressure, mingas_depth;
		const char *unit, *pressure_unit, *depth_unit;
		std::string temp;
		std::string warning;
		std::string mingas;
		if (cyl.cylinder_use == NOT_USED)
			continue;

		volume = get_volume_units(cyl.gas_used.mliter, NULL, &unit);
		deco_volume = get_volume_units(cyl.deco_gas_used.mliter, NULL, &unit);
		if (cyl.type.size.mliter) {
			int remaining_gas = lrint((double)cyl.end.mbar * cyl.type.size.mliter / 1000.0 / gas_compressibility_factor(cyl.gasmix, cyl.end.mbar / 1000.0));
			double deco_pressure_mbar = isothermal_pressure(cyl.gasmix, 1.0, remaining_gas + cyl.deco_gas_used.mliter,
				cyl.type.size.mliter) * 1000 - cyl.end.mbar;
			deco_pressure = get_pressure_units(lrint(deco_pressure_mbar), &pressure_unit);
			pressure = get_pressure_units(cyl.start.mbar - cyl.end.mbar, &pressure_unit);
			/* Warn if the plan uses more gas than is available in a cylinder
			 * This only works if we have working pressure for the cylinder
			 * 10bar is a made up number - but it seemed silly to pretend you could breathe cylinder down to 0 */
			if (cyl.end.mbar < 10000)
				warning = format_string_std("<br/>\n&nbsp;&mdash; <span style='color: red;'>%s </span> %s",
					translate("gettextFromC", "Warning:"),
					translate("gettextFromC", "this is more gas than available in the specified cylinder!"));
			else
				if (cyl.end.mbar / 1000.0 * cyl.type.size.mliter / gas_compressibility_factor(cyl.gasmix, cyl.end.mbar / 1000.0)
				    < cyl.deco_gas_used.mliter)
					warning = format_string_std("<br/>\n&nbsp;&mdash; <span style='color: red;'>%s </span> %s",
						translate("gettextFromC", "Warning:"),
						translate("gettextFromC", "not enough reserve for gas sharing on ascent!"));

			/* Do and print minimum gas calculation for last bottom gas, but only for OC mode, */
			/* not for recreational mode and if no other warning was set before. */
			else
				if (lastbottomdp && gasidx == lastbottomdp->cylinderid
					&& dive.dcs[0].divemode == OC && decoMode(true) != RECREATIONAL) {
					/* Calculate minimum gas volume. */
					volume_t mingasv;
					mingasv.mliter = lrint(prefs.sacfactor / 100.0 * prefs.problemsolvingtime * prefs.bottomsac
						* dive.depth_to_bar(lastbottomdp->depth)
						+ prefs.sacfactor / 100.0 * cyl.deco_gas_used.mliter);
					/* Calculate minimum gas pressure for cyclinder. */
					lastbottomdp->minimum_gas.mbar = lrint(isothermal_pressure(cyl.gasmix, 1.0,
						mingasv.mliter, cyl.type.size.mliter) * 1000);
					/* Translate all results into correct units */
					mingas_volume = get_volume_units(mingasv.mliter, NULL, &unit);
					mingas_pressure = get_pressure_units(lastbottomdp->minimum_gas.mbar, &pressure_unit);
					mingas_d_pressure = get_pressure_units(lrint((double)cyl.end.mbar + deco_pressure_mbar - lastbottomdp->minimum_gas.mbar), &pressure_unit);
					mingas_depth = get_depth_units(lastbottomdp->depth, NULL, &depth_unit);
					/* Print it to results */
					if (cyl.start.mbar > lastbottomdp->minimum_gas.mbar) {
						mingas = casprintf_loc("<br/>\n&nbsp;&mdash; <span style='color: %s;'>%s</span> (%s %.1fx%s/+%d%s@%.0f%s): "
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
						warning = format_string_std("<br/>\n&nbsp;&mdash; <span style='color: red;'>%s </span> %s",
							translate("gettextFromC", "Warning:"),
							translate("gettextFromC", "required minimum gas for ascent already exceeding start pressure of cylinder!"));
					}
				}
			/* Print the gas consumption for every cylinder here to temp buffer. */
			if (lrint(volume) > 0) {
				temp = casprintf_loc(translate("gettextFromC", "%.0f%s/%.0f%s of <span style='color: red;'><b>%s</b></span> (%.0f%s/%.0f%s in planned ascent)"),
					     volume, unit, pressure, pressure_unit, cyl.gasmix.name().c_str(), deco_volume, unit, deco_pressure, pressure_unit);
			} else {
				temp = casprintf_loc(translate("gettextFromC", "%.0f%s/%.0f%s of <span style='color: red;'><b>%s</b></span>"),
					     volume, unit, pressure, pressure_unit, cyl.gasmix.name().c_str());
			}
		} else {
			if (lrint(volume) > 0) {
				temp = casprintf_loc(translate("gettextFromC", "%.0f%s of <span style='color: red;'><b>%s</b></span> (%.0f%s during planned ascent)"),
					     volume, unit, cyl.gasmix.name().c_str(), deco_volume, unit);
			} else {
				temp = casprintf_loc(translate("gettextFromC", "%.0f%s of <span style='color: red;'><b>%s</b></span>"),
					     volume, unit, cyl.gasmix.name().c_str());
			}
		}
		/* Gas consumption: Now finally print all strings to output */
		buf += format_string_std("%s%s%s<br/>\n", temp.c_str(), warning.c_str(), mingas.c_str());
	}
	buf += "</div>\n";

	/* For trimix OC dives, if an icd table header and icd data were printed to buffer, then add the ICD table here */
	if (!icdtableheader && prefs.show_icd) {
		icdbuf += "</tbody></table>\n"; // End the ICD table
		buf += icdbuf;
		if (icdwarning) { // If necessary, add warning
			buf += format_string_std("<span style='color: red;'>%s</span> %s",
				translate("gettextFromC", "Warning:"),
				translate("gettextFromC", "Isobaric counterdiffusion conditions exceeded"));
		}
		buf += "<br/></div>\n";
	}

	/* Print warnings for pO2 (move into separate function?) */
	{
		bool o2warning_exist = false;
		double amb;

		divemode_loop loop(dive.dcs[0]);
		if (dive.dcs[0].divemode != CCR) {
			for (auto &dp: this->dp) {
				if (dp.time != 0) {
					std::string temp;
					struct gasmix gasmix = dive.get_cylinder(dp.cylinderid)->gasmix;

					divemode_t current_divemode = loop.at(dp.time).first;
					amb = dive.depth_to_atm(dp.depth);
					gas_pressures pressures = fill_pressures(amb, gasmix, (current_divemode == OC) ? 0.0 : amb * gasmix.o2.permille / 1000.0, current_divemode);

					if (pressures.o2 > (dp.entered ? prefs.bottompo2 : prefs.decopo2) / 1000.0) {
						const char *depth_unit;
						int decimals;
						double depth_value = get_depth_units(dp.depth, &decimals, &depth_unit);
						if (!o2warning_exist)
							buf += "<div>\n";
						o2warning_exist = true;
						temp = casprintf_loc(translate("gettextFromC", "high pO₂ value %.3f bar at %d:%02u with gas %s at depth %.*f %s"),
							pressures.o2, FRACTION_TUPLE(dp.time, 60), gasmix.name().c_str(), decimals, depth_value, depth_unit);
						buf += format_string_std("<span style='color: red;'>%s </span> %s<br/>\n", translate("gettextFromC", "Warning:"), temp.c_str());
					} else if (pressures.o2 < 0.16) {
						const char *depth_unit;
						int decimals;
						double depth_value = get_depth_units(dp.depth, &decimals, &depth_unit);
						if (!o2warning_exist)
							buf += "<div>";
						o2warning_exist = true;
						temp = casprintf_loc(translate("gettextFromC", "low pO₂ value %.3f bar at %d:%02u with gas %s at depth %.*f %s"),
							pressures.o2, FRACTION_TUPLE(dp.time, 60), gasmix.name().c_str(), decimals, depth_value, depth_unit);
						buf += format_string_std("<span style='color: red;'>%s </span> %s<br/>\n", translate("gettextFromC", "Warning:"), temp.c_str());
					}
				}
			}
		}
		if (o2warning_exist)
			buf += "</div>\n";
	}
	dive.notes = std::move(buf);
#ifdef DEBUG_PLANNER_NOTES
	printf("<!DOCTYPE html>\n<html>\n\t<head><title>plannernotes</title><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/></head>\n\t<body>\n%s\t</body>\n</html>\n", dive.notes);
#endif
}
