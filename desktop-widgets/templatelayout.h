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

#define _CONC_STR(prop, idx)   _CONC_STR1(prop, idx)
#define _CONC_STR1(prop, idx)  _CONC_STR2(prop ## idx)
#define _CONC_STR2(prop)       #prop

#define _RETURN_DIVE_PROPERTY(prop) \
	if (property == #prop) return object.prop()

#define _RETURN_DIVE_PROPERTY_IDX(prop, idx) \
	if (property == _CONC_STR(prop, idx)) return object.prop(idx)

GRANTLEE_BEGIN_LOOKUP(Dive)
_RETURN_DIVE_PROPERTY(number);
else _RETURN_DIVE_PROPERTY(id);
else _RETURN_DIVE_PROPERTY(date);
else _RETURN_DIVE_PROPERTY(time);
else _RETURN_DIVE_PROPERTY(location);
else _RETURN_DIVE_PROPERTY(duration);
else _RETURN_DIVE_PROPERTY(depth);
else _RETURN_DIVE_PROPERTY(buddy);
else _RETURN_DIVE_PROPERTY(divemaster);
else _RETURN_DIVE_PROPERTY(airTemp);
else _RETURN_DIVE_PROPERTY(waterTemp);
else _RETURN_DIVE_PROPERTY(notes);
else _RETURN_DIVE_PROPERTY(rating);
else _RETURN_DIVE_PROPERTY(sac);
else _RETURN_DIVE_PROPERTY(tags);
else _RETURN_DIVE_PROPERTY(gas);
else _RETURN_DIVE_PROPERTY(suit);
else _RETURN_DIVE_PROPERTY(cylinders);
else _RETURN_DIVE_PROPERTY_IDX(cylinder, 0);
else _RETURN_DIVE_PROPERTY_IDX(cylinder, 1);
else _RETURN_DIVE_PROPERTY_IDX(cylinder, 2);
else _RETURN_DIVE_PROPERTY_IDX(cylinder, 3);
else _RETURN_DIVE_PROPERTY_IDX(cylinder, 4);
else _RETURN_DIVE_PROPERTY_IDX(cylinder, 5);
else _RETURN_DIVE_PROPERTY_IDX(cylinder, 6);
else _RETURN_DIVE_PROPERTY_IDX(cylinder, 7);
else _RETURN_DIVE_PROPERTY(weights);
else _RETURN_DIVE_PROPERTY_IDX(weight, 0);
else _RETURN_DIVE_PROPERTY_IDX(weight, 1);
else _RETURN_DIVE_PROPERTY_IDX(weight, 2);
else _RETURN_DIVE_PROPERTY_IDX(weight, 3);
else _RETURN_DIVE_PROPERTY_IDX(weight, 4);
else _RETURN_DIVE_PROPERTY_IDX(weight, 5);
else _RETURN_DIVE_PROPERTY(maxcns);
else _RETURN_DIVE_PROPERTY(otu);
GRANTLEE_END_LOOKUP

#undef _RETURN_DIVE_PROPERTY
#undef _RETURN_DIVE_PROPERTY_IDX
#undef _CONC_STR
#undef _CONC_STR1
#undef _CONC_STR2

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
