// SPDX-License-Identifier: GPL-2.0
#ifndef TEMPLATELAYOUT_H
#define TEMPLATELAYOUT_H

#include <QStringList>
#include <grantlee_templates.h>
#include "mainwindow.h"
#include "printoptions.h"
#include "core/statistics.h"
#include "core/qthelper.h"
#include "core/subsurface-qt/DiveObjectHelper.h"
#include "core/subsurface-qt/CylinderObjectHelper.h" // TODO: remove once grantlee supports Q_GADGET objects

int getTotalWork(print_options *printOptions);
void find_all_templates();
void set_bundled_templates_as_read_only();
void copy_bundled_templates(QString src, QString dst, QStringList *templateBackupList);

extern QList<QString> grantlee_templates, grantlee_statistics_templates;

class TemplateLayout : public QObject {
	Q_OBJECT
public:
	TemplateLayout(print_options *printOptions, template_options *templateOptions);
	QString generate();
	QString generateStatistics();
	static QString readTemplate(QString template_name);
	static void writeTemplate(QString template_name, QString grantlee_template);

private:
	print_options *printOptions;
	template_options *templateOptions;

signals:
	void progressUpdated(int value);
};

struct YearInfo {
	stats_t *year;
};

Q_DECLARE_METATYPE(template_options)
Q_DECLARE_METATYPE(print_options)
Q_DECLARE_METATYPE(YearInfo)

GRANTLEE_BEGIN_LOOKUP(template_options)
if (property == "font") {
	switch (object.font_index) {
	case 0:
		return "Arial, Helvetica, sans-serif";
	case 1:
		return "Impact, Charcoal, sans-serif";
	case 2:
		return "Georgia, serif";
	case 3:
		return "Courier, monospace";
	case 4:
		return "Verdana, Geneva, sans-serif";
	}
} else if (property == "borderwidth") {
	return object.border_width;
} else if (property == "font_size") {
	return object.font_size / 9.0;
} else if (property == "line_spacing") {
	return object.line_spacing;
} else if (property == "color1") {
	return object.color_palette.color1.name();
} else if (property == "color2") {
	return object.color_palette.color2.name();
} else if (property == "color3") {
	return object.color_palette.color3.name();
} else if (property == "color4") {
	return object.color_palette.color4.name();
} else if (property == "color5") {
	return object.color_palette.color5.name();
} else if (property == "color6") {
	return object.color_palette.color6.name();
}
GRANTLEE_END_LOOKUP

GRANTLEE_BEGIN_LOOKUP(print_options)
if (property == "grayscale") {
	if (object.color_selected) {
		return "";
	} else {
		return "-webkit-filter: grayscale(100%)";
	}
}
GRANTLEE_END_LOOKUP

GRANTLEE_BEGIN_LOOKUP(YearInfo)
if (property == "year") {
	return object.year->period;
} else if (property == "dives") {
	return object.year->selection_size;
} else if (property == "min_temp") {
	return object.year->min_temp.mkelvin == 0 ? "0" : get_temperature_string(object.year->min_temp, true);
} else if (property == "max_temp") {
	return object.year->max_temp.mkelvin == 0 ? "0" : get_temperature_string(object.year->max_temp, true);
} else if (property == "total_time") {
	return get_dive_duration_string(object.year->total_time.seconds, gettextFromC::tr("h"),
									gettextFromC::tr("min"), gettextFromC::tr("sec"), " ");
} else if (property == "avg_time") {
	return get_minutes(object.year->total_time.seconds / object.year->selection_size);
} else if (property == "shortest_time") {
	return get_minutes(object.year->shortest_time.seconds);
} else if (property == "longest_time") {
	return get_minutes(object.year->longest_time.seconds);
} else if (property == "avg_depth") {
	return get_depth_string(object.year->avg_depth);
} else if (property == "min_depth") {
	return get_depth_string(object.year->min_depth);
} else if (property == "max_depth") {
	return get_depth_string(object.year->max_depth);
} else if (property == "avg_sac") {
	return get_volume_string(object.year->avg_sac);
} else if (property == "min_sac") {
	return get_volume_string(object.year->min_sac);
} else if (property == "max_sac") {
	return get_volume_string(object.year->max_sac);
}
GRANTLEE_END_LOOKUP

// TODO: This is currently needed because our grantlee version
// doesn't support Q_GADGET based classes. A patch to fix this
// exists. Remove in due course.
GRANTLEE_BEGIN_LOOKUP(CylinderObjectHelper)
if (property == "description") {
	return object.description;
} else if (property == "size") {
	return object.size;
} else if (property == "workingPressure") {
	return object.workingPressure;
} else if (property == "startPressure") {
	return object.startPressure;
} else if (property == "endPressure") {
	return object.endPressure;
} else if (property == "gasMix") {
	return object.gasMix;
}
GRANTLEE_END_LOOKUP

// TODO: This is currently needed because our grantlee version
// doesn't support Q_GADGET based classes. A patch to fix this
// exists. Remove in due course.
GRANTLEE_BEGIN_LOOKUP(DiveObjectHelper)
if (property == "number") {
	return object.number;
} else if (property == "id") {
	return object.id;
} else if (property == "rating") {
	return object.rating;
} else if (property == "visibility") {
	return object.visibility;
} else if (property == "date") {
	return object.date();
} else if (property == "time") {
	return object.time();
} else if (property == "timestamp") {
	return QVariant::fromValue(object.timestamp);
} else if (property == "location") {
	return object.location;
} else if (property == "gps") {
	return object.gps;
} else if (property == "gps_decimal") {
	return object.gps_decimal;
} else if (property == "dive_site") {
	return object.dive_site;
} else if (property == "duration") {
	return object.duration;
} else if (property == "noDive") {
	return object.noDive;
} else if (property == "depth") {
	return object.depth;
} else if (property == "divemaster") {
	return object.divemaster;
} else if (property == "buddy") {
	return object.buddy;
} else if (property == "airTemp") {
	return object.airTemp;
} else if (property == "waterTemp") {
	return object.waterTemp;
} else if (property == "notes") {
	return object.notes;
} else if (property == "tags") {
	return object.tags;
} else if (property == "gas") {
	return object.gas;
} else if (property == "sac") {
	return object.sac;
} else if (property == "weightList") {
	return object.weightList;
} else if (property == "weights") {
	return object.weights;
} else if (property == "singleWeight") {
	return object.singleWeight;
} else if (property == "suit") {
	return object.suit;
} else if (property == "cylinderList") {
	return object.cylinderList();
} else if (property == "cylinders") {
	return object.cylinders;
} else if (property == "cylinderObjects") {
	return QVariant::fromValue(object.cylinderObjects);
} else if (property == "maxcns") {
	return object.maxcns;
} else if (property == "otu") {
	return object.otu;
} else if (property == "sumWeight") {
	return object.sumWeight;
} else if (property == "getCylinder") {
	return object.getCylinder;
} else if (property == "startPressure") {
	return object.startPressure;
} else if (property == "endPressure") {
	return object.endPressure;
} else if (property == "firstGas") {
	return object.firstGas;
}
GRANTLEE_END_LOOKUP
#endif
