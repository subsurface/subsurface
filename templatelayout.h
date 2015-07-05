#ifndef TEMPLATELAYOUT_H
#define TEMPLATELAYOUT_H

#include <grantlee_templates.h>
#include "mainwindow.h"
#include "printoptions.h"
#include "templateedit.h"

int getTotalWork(print_options *printOptions);

class TemplateLayout : public QObject {
	Q_OBJECT
public:
	TemplateLayout(print_options *PrintOptions, template_options *templateOptions);
	~TemplateLayout();
	QString generate();
	static QString readTemplate(QString template_name);
	static void writeTemplate(QString template_name, QString grantlee_template);

private:
	Grantlee::Engine *m_engine;
	print_options *PrintOptions;
	template_options *templateOptions;

signals:
	void progressUpdated(int value);
};

class Dive {
private:
	int m_number;
	QString m_date;
	QString m_time;
	QString m_location;
	QString m_duration;
	QString m_depth;
	QString m_divemaster;
	QString m_buddy;
	QString m_airTemp;
	QString m_waterTemp;
	QString m_notes;
	struct dive *dive;
	void put_date_time();
	void put_location();
	void put_duration();
	void put_depth();
	void put_divemaster();
	void put_buddy();
	void put_temp();
	void put_notes();

public:
	Dive(struct dive *dive)
	    : dive(dive)
	{
		m_number = dive->number;
		put_date_time();
		put_location();
		put_duration();
		put_depth();
		put_divemaster();
		put_buddy();
		put_temp();
		put_notes();
	}
	Dive();
	~Dive();
	int number() const;
	QString date() const;
	QString time() const;
	QString location() const;
	QString duration() const;
	QString depth() const;
	QString divemaster() const;
	QString buddy() const;
	QString airTemp() const;
	QString waterTemp() const;
	QString notes() const;
};

Q_DECLARE_METATYPE(Dive)
Q_DECLARE_METATYPE(template_options)

GRANTLEE_BEGIN_LOOKUP(Dive)
if (property == "number")
	return object.number();
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
} else if (property == "font_size") {
	return object.font_size / 9.0;
} else if (property == "line_spacing") {
	return object.line_spacing;
}
GRANTLEE_END_LOOKUP

#endif
