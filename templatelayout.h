#ifndef TEMPLATELAYOUT_H
#define TEMPLATELAYOUT_H

#include <grantlee_templates.h>
#include "mainwindow.h"
#include "printoptions.h"
#include "statistics.h"
#include "qthelper.h"
#include "helpers.h"

int getTotalWork(print_options *printOptions);
void find_all_templates();

extern QList<QString> grantlee_templates, grantlee_statistics_templates;

class TemplateLayout : public QObject {
	Q_OBJECT
public:
	TemplateLayout(print_options *PrintOptions, template_options *templateOptions);
	~TemplateLayout();
	QString generate();
	QString generateStatistics();
	static QString readTemplate(QString template_name);
	static void writeTemplate(QString template_name, QString grantlee_template);

private:
	Grantlee::Engine *m_engine;
	print_options *PrintOptions;
	template_options *templateOptions;

signals:
	void progressUpdated(int value);
};

class YearInfo {
public:
	stats_t *year;
	YearInfo(stats_t& year)
		:year(&year)
	{

	}
	YearInfo();
	~YearInfo();
};

Q_DECLARE_METATYPE(Dive)
Q_DECLARE_METATYPE(template_options)
Q_DECLARE_METATYPE(print_options)
Q_DECLARE_METATYPE(YearInfo)

GRANTLEE_BEGIN_LOOKUP(Dive)
if (property == "number")
	return object.number();
else if (property == "id")
	return object.id();
else if (property == "date")
	return object.date();
else if (property == "time")
	return object.time();
else if (property == "location")
	return object.location();
else if (property == "duration")
	return object.duration();
else if (property == "depth")
	return object.depth();
else if (property == "divemaster")
	return object.divemaster();
else if (property == "buddy")
	return object.buddy();
else if (property == "airTemp")
	return object.airTemp();
else if (property == "waterTemp")
	return object.waterTemp();
else if (property == "notes")
	return object.notes();
else if (property == "rating")
	return object.rating();
else if (property == "sac")
	return object.sac();
else if (property == "tags")
	return object.tags();
else if (property == "gas")
	return object.gas();
GRANTLEE_END_LOOKUP

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
	const char *unit;
	double temp = get_temp_units(object.year->min_temp, &unit);
	return object.year->min_temp == 0 ? "0" : QString::number(temp, 'g', 2) + unit;
} else if (property == "max_temp") {
	const char *unit;
	double temp = get_temp_units(object.year->max_temp, &unit);
	return object.year->max_temp == 0 ? "0" : QString::number(temp, 'g', 2) + unit;
} else if (property == "total_time") {
	return get_time_string(object.year->total_time.seconds, 0);
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

#endif
