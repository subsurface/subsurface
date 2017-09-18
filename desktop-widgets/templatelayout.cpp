// SPDX-License-Identifier: GPL-2.0
#include <string>

#include "templatelayout.h"
#include "core/helpers.h"
#include "core/display.h"

QList<QString> grantlee_templates, grantlee_statistics_templates;

int getTotalWork(print_options *printOptions)
{
	if (printOptions->print_selected) {
		// return the correct number depending on all/selected dives
		// but don't return 0 as we might divide by this number
		return amount_selected && !in_planner() ? amount_selected : 1;
	}
	return dive_table.nr;
}

void find_all_templates()
{
	grantlee_templates.clear();
	grantlee_statistics_templates.clear();
	QDir dir(getPrintingTemplatePathUser());
	QStringList list = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
	foreach (const QString& filename, list) {
		if (filename.at(filename.size() - 1) != '~') {
			grantlee_templates.append(filename);
		}
	}

	// find statistics templates
	dir.setPath(getPrintingTemplatePathUser() + QDir::separator() + "statistics");
	list = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
	foreach (const QString& filename, list) {
		if (filename.at(filename.size() - 1) != '~') {
			grantlee_statistics_templates.append(filename);
		}
	}
}

TemplateLayout::TemplateLayout(print_options *PrintOptions, template_options *templateOptions) :
	m_engine(NULL)
{
	this->PrintOptions = PrintOptions;
	this->templateOptions = templateOptions;
}

TemplateLayout::~TemplateLayout()
{
	delete m_engine;
}

/* a HTML pre-processor stage. acts like a compatibility layer
 * between some Grantlee variables and DiveObjectHelper Q_PROPERTIES;
 */
static QString preprocessTemplate(const QString &in)
{
	int i;
	QString out = in;
	QString iStr;
	QList<QPair<QString, QString> > list;

	/* populate known variables in a QPair list */
	list << qMakePair(QString("dive.weights"), QString("dive.weightList"));
	for (i = 0; i < MAX_WEIGHTSYSTEMS; i++)
		list << qMakePair(QString("dive.weight%1").arg(i), QString("dive.weights.%1").arg(i));

	list << qMakePair(QString("dive.cylinders"), QString("dive.cylinderList"));
	for (i = 0; i < MAX_CYLINDERS; i++)
		list << qMakePair(QString("dive.cylinder%1").arg(i), QString("dive.cylinders.%1").arg(i));

	/* lazy method of variable replacement without regex. the Grantlee parser
	 * works with a single or no space next to the variable markers -
	 * e.g. '{{ var }}' */
	for (i = 0; i < list.length(); i++) {
		QPair<QString, QString> p = list.at(i);
		out.replace("{{ " + p.first + " }}", "{{ " + p.second + " }}");
		out.replace("{{" + p.first + "}}", "{{" + p.second + "}}");
		out.replace("{{ " + p.first + "}}", "{{ " + p.second + "}}");
		out.replace("{{" + p.first + " }}", "{{" + p.second + " }}");
	}

	return out;
}

QString TemplateLayout::generate()
{
	int progress = 0;
	int totalWork = getTotalWork(PrintOptions);

	QString htmlContent;
	delete m_engine;
	m_engine = new Grantlee::Engine(this);
	Grantlee::registerMetaType<template_options>();
	Grantlee::registerMetaType<print_options>();

	QVariantList diveList;

	struct dive *dive;
	if (in_planner()) {
		DiveObjectHelper *d = new DiveObjectHelper(&displayed_dive);
		diveList.append(QVariant::fromValue(d));
		emit progressUpdated(100.0);
	} else {
		int i;
		for_each_dive (i, dive) {
			//TODO check for exporting selected dives only
			if (!dive->selected && PrintOptions->print_selected)
				continue;
			DiveObjectHelper *d = new DiveObjectHelper(dive);
			diveList.append(QVariant::fromValue(d));
			progress++;
			emit progressUpdated(lrint(progress * 100.0 / totalWork));
		}
	}
	Grantlee::Context c;
	c.insert("dives", diveList);
	c.insert("template_options", QVariant::fromValue(*templateOptions));
	c.insert("print_options", QVariant::fromValue(*PrintOptions));

	/* don't use the Grantlee loader API */
	QString templateContents = readTemplate(PrintOptions->p_template);
	QString preprocessed = preprocessTemplate(templateContents);

	/* create the template from QString; is this thing allocating memory? */
	Grantlee::Template t = m_engine->newTemplate(preprocessed, PrintOptions->p_template);
	if (!t || t->error()) {
		qDebug() << "Can't load template";
		return htmlContent;
	}

	htmlContent = t->render(&c);

	if (t->error()) {
		qDebug() << "Can't render template";
	}
	return htmlContent;
}

QString TemplateLayout::generateStatistics()
{
	QString htmlContent;
	delete m_engine;
	m_engine = new Grantlee::Engine(this);

	QSharedPointer<Grantlee::FileSystemTemplateLoader> m_templateLoader =
		QSharedPointer<Grantlee::FileSystemTemplateLoader>(new Grantlee::FileSystemTemplateLoader());
	m_templateLoader->setTemplateDirs(QStringList() << getPrintingTemplatePathUser() + QDir::separator() + QString("statistics"));
	m_engine->addTemplateLoader(m_templateLoader);

	Grantlee::registerMetaType<YearInfo>();
	Grantlee::registerMetaType<template_options>();
	Grantlee::registerMetaType<print_options>();

	QVariantList years;

	int i = 0;
	while (stats_yearly != NULL && stats_yearly[i].period) {
		YearInfo year(stats_yearly[i]);
		years.append(QVariant::fromValue(year));
		i++;
	}

	Grantlee::Context c;
	c.insert("years", years);
	c.insert("template_options", QVariant::fromValue(*templateOptions));
	c.insert("print_options", QVariant::fromValue(*PrintOptions));

	Grantlee::Template t = m_engine->loadByName(PrintOptions->p_template);
	if (!t || t->error()) {
		qDebug() << "Can't load template";
		return htmlContent;
	}

	htmlContent = t->render(&c);

	if (t->error()) {
		qDebug() << "Can't render template";
		return htmlContent;
	}

	emit progressUpdated(100);
	return htmlContent;
}

QString TemplateLayout::readTemplate(QString template_name)
{
	QFile qfile(getPrintingTemplatePathUser() + QDir::separator() + template_name);
	if (qfile.open(QFile::ReadOnly | QFile::Text)) {
		QTextStream in(&qfile);
		return in.readAll();
	}
	return "";
}

void TemplateLayout::writeTemplate(QString template_name, QString grantlee_template)
{
	QFile qfile(getPrintingTemplatePathUser() + QDir::separator() + template_name);
	if (qfile.open(QFile::ReadWrite | QFile::Text)) {
		qfile.write(grantlee_template.toUtf8().data());
		qfile.resize(qfile.pos());
		qfile.close();
	}
}

YearInfo::YearInfo()
{

}

YearInfo::~YearInfo()
{

}
