#include <string>

#include "templatelayout.h"
#include "helpers.h"
#include "display.h"

QList<QString> grantlee_templates, grantlee_statistics_templates;

int getTotalWork(print_options *printOptions)
{
	if (printOptions->print_selected) {
		// return the correct number depending on all/selected dives
		// but don't return 0 as we might divide by this number
		return amount_selected ? amount_selected : 1;
	}
	int dives = 0, i;
	struct dive *dive;
	for_each_dive (i, dive) {
		dives++;
	}
	return dives;
}

void find_all_templates()
{
	grantlee_templates.clear();
	grantlee_statistics_templates.clear();
	QDir dir(getPrintingTemplatePathUser());
	QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
	foreach (QFileInfo finfo, list) {
		QString filename = finfo.fileName();
		if (filename.at(filename.size() - 1) != '~') {
			grantlee_templates.append(finfo.fileName());
		}
	}
	// find statistics templates
	dir.setPath(getPrintingTemplatePathUser() + QDir::separator() + "statistics");
	list = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot);
	foreach (QFileInfo finfo, list) {
		QString filename = finfo.fileName();
		if (filename.at(filename.size() - 1) != '~') {
			grantlee_statistics_templates.append(finfo.fileName());
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

QString TemplateLayout::generate()
{
	int progress = 0;
	int totalWork = getTotalWork(PrintOptions);

	QString htmlContent;
	m_engine = new Grantlee::Engine(this);

	QSharedPointer<Grantlee::FileSystemTemplateLoader> m_templateLoader =
		QSharedPointer<Grantlee::FileSystemTemplateLoader>(new Grantlee::FileSystemTemplateLoader());
	m_templateLoader->setTemplateDirs(QStringList() << getPrintingTemplatePathUser());
	m_engine->addTemplateLoader(m_templateLoader);

	Grantlee::registerMetaType<Dive>();
	Grantlee::registerMetaType<template_options>();
	Grantlee::registerMetaType<print_options>();

	QVariantList diveList;

	struct dive *dive;
	int i;
	for_each_dive (i, dive) {
		//TODO check for exporting selected dives only
		if (!dive->selected && PrintOptions->print_selected)
			continue;
		Dive d(dive);
		diveList.append(QVariant::fromValue(d));
		progress++;
		emit progressUpdated(progress * 100.0 / totalWork);
	}
	Grantlee::Context c;
	c.insert("dives", diveList);
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
	return htmlContent;
}

QString TemplateLayout::generateStatistics()
{
	QString htmlContent;
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
