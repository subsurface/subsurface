// SPDX-License-Identifier: GPL-2.0
#include <QFileDevice>
#include <QRegularExpression>
#include <list>

#include "templatelayout.h"
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
	const QLatin1String ext(".html");
	grantlee_templates.clear();
	grantlee_statistics_templates.clear();
	QDir dir(getPrintingTemplatePathUser());
	const QStringList list = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
	for (const QString &filename: list) {
		if (filename.at(filename.size() - 1) != '~' && filename.endsWith(ext))
			grantlee_templates.append(filename);
	}

	// find statistics templates
	dir.setPath(getPrintingTemplatePathUser() + QDir::separator() + "statistics");
	const QStringList stat = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
	for (const QString &filename: stat) {
		if (filename.at(filename.size() - 1) != '~' && filename.endsWith(ext))
			grantlee_statistics_templates.append(filename);
	}
}

/* find templates which are part of the bundle in the user path
 * and set them as read only.
 */
void set_bundled_templates_as_read_only()
{
	QDir dir;
	const QString stats("statistics");
	QStringList list, listStats;
	QString pathBundle = getPrintingTemplatePathBundle();
	QString pathUser = getPrintingTemplatePathUser();

	dir.setPath(pathBundle);
	list = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
	dir.setPath(pathBundle + QDir::separator() + stats);
	listStats = dir.entryList(QDir::Files | QDir::NoDotAndDotDot);
	for (int i = 0; i < listStats.length(); i++)
		listStats[i] = stats + QDir::separator() + listStats.at(i);
	list += listStats;

	foreach (const QString& f, list)
		QFile::setPermissions(pathUser + QDir::separator() + f, QFileDevice::ReadOwner | QFileDevice::ReadUser);
}

void copy_bundled_templates(QString src, QString dst, QStringList *templateBackupList)
{
	QDir dir(src);
	if (!dir.exists())
		return;
	const auto dirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	for (const QString &d: dirs) {
		QString dst_path = dst + QDir::separator() + d;
		dir.mkpath(dst_path);
		copy_bundled_templates(src + QDir::separator() + d, dst_path, templateBackupList);
	}
	const auto files = dir.entryList(QDir::Files);
	for (const QString &f: files) {
		QFile fileSrc(src + QDir::separator() + f);
		QFile fileDest(dst + QDir::separator() + f);
		if (fileDest.exists()) {
			// if open() fails the file is either locked or r/o. try to remove it and then overwrite
			if (!fileDest.open(QFile::ReadWrite | QFile::Text)) {
				fileDest.setPermissions(QFileDevice::WriteOwner | QFileDevice::WriteUser);
				fileDest.remove();
			} else { // if the file is not read-only create a backup
				fileDest.close();
				const QString targetFile = fileDest.fileName().replace(".html", "-User.html");
				fileDest.copy(targetFile);
				*templateBackupList << targetFile;
			}
		}
		fileSrc.copy(fileDest.fileName()); // in all cases copy the file
	}
}

TemplateLayout::TemplateLayout(print_options *printOptions, template_options *templateOptions)
{
	this->printOptions = printOptions;
	this->templateOptions = templateOptions;
}

/* a HTML pre-processor stage. acts like a compatibility layer
 * between some Grantlee variables and DiveObjectHelperGrantlee Q_PROPERTIES:
 *	dive.weights -> dive.weightList
 * 	dive.weight# -> dive.weights.#
 *	dive.cylinders -> dive.cylinderList
 * 	dive.cylinder# -> dive.cylinders.#
 * The Grantlee parser works with a single or no space next to the variable
 * markers - e.g. '{{ var }}'. We're graceful and support an arbitrary number of
 * whitespace. */
static QRegularExpression weightsRegExp(R"({{\*?([A-Za-z]+[A-Za-z0-9]*).weights\s*}})");
static QRegularExpression weightRegExp(R"({{\*?([A-Za-z]+[A-Za-z0-9]*).weight(\d+)\s*}})");
static QRegularExpression cylindersRegExp(R"({{\*?([A-Za-z]+[A-Za-z0-9]*).cylinders\s*}})");
static QRegularExpression cylinderRegExp(R"({{\s*([A-Za-z]+[A-Za-z0-9]*).cylinder(\d+)\s*}})");
static QString preprocessTemplate(const QString &in)
{
	QString out = in;

	out.replace(weightsRegExp, QStringLiteral(R"({{\1.weightList}})"));
	out.replace(weightRegExp, QStringLiteral(R"({{\1.weights.\2}})"));
	out.replace(cylindersRegExp, QStringLiteral(R"({{\1.cylinderList}})"));
	out.replace(cylindersRegExp, QStringLiteral(R"({{\1.cylinders.\2}})"));

	return out;
}

QString TemplateLayout::generate()
{
	int progress = 0;
	int totalWork = getTotalWork(printOptions);

	QString htmlContent;
	Grantlee::Engine engine(this);
	Grantlee::registerMetaType<template_options>();
	Grantlee::registerMetaType<print_options>();
	Grantlee::registerMetaType<CylinderObjectHelper>(); // TODO: Remove when grantlee supports Q_GADGET
	Grantlee::registerMetaType<DiveObjectHelperGrantlee>(); // TODO: Remove when grantlee supports Q_GADGET

	QVariantList diveList;

	struct dive *dive;
	if (in_planner()) {
		diveList.append(QVariant::fromValue(DiveObjectHelperGrantlee(&displayed_dive)));
		emit progressUpdated(100.0);
	} else {
		int i;
		for_each_dive (i, dive) {
			//TODO check for exporting selected dives only
			if (!dive->selected && printOptions->print_selected)
				continue;
			diveList.append(QVariant::fromValue(DiveObjectHelperGrantlee(dive)));
			progress++;
			emit progressUpdated(lrint(progress * 100.0 / totalWork));
		}
	}
	Grantlee::Context c;
	c.insert("dives", diveList);
	c.insert("template_options", QVariant::fromValue(*templateOptions));
	c.insert("print_options", QVariant::fromValue(*printOptions));

	/* don't use the Grantlee loader API */
	QString templateContents = readTemplate(printOptions->p_template);
	QString preprocessed = preprocessTemplate(templateContents);

	/* create the template from QString; is this thing allocating memory? */
	Grantlee::Template t = engine.newTemplate(preprocessed, printOptions->p_template);
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
	Grantlee::Engine engine(this);

	QSharedPointer<Grantlee::FileSystemTemplateLoader> m_templateLoader =
		QSharedPointer<Grantlee::FileSystemTemplateLoader>(new Grantlee::FileSystemTemplateLoader());
	m_templateLoader->setTemplateDirs(QStringList() << getPrintingTemplatePathUser() + QDir::separator() + QString("statistics"));
	engine.addTemplateLoader(m_templateLoader);

	Grantlee::registerMetaType<YearInfo>();
	Grantlee::registerMetaType<template_options>();
	Grantlee::registerMetaType<print_options>();
	Grantlee::registerMetaType<CylinderObjectHelper>(); // TODO: Remove when grantlee supports Q_GADGET
	Grantlee::registerMetaType<DiveObjectHelperGrantlee>(); // TODO: Remove when grantlee supports Q_GADGET

	QVariantList years;

	int i = 0;
	stats_summary_auto_free stats;
	calculate_stats_summary(&stats, false);
	while (stats.stats_yearly != NULL && stats.stats_yearly[i].period) {
		YearInfo year{ &stats.stats_yearly[i] };
		years.append(QVariant::fromValue(year));
		i++;
	}

	Grantlee::Context c;
	c.insert("years", years);
	c.insert("template_options", QVariant::fromValue(*templateOptions));
	c.insert("print_options", QVariant::fromValue(*printOptions));

	Grantlee::Template t = engine.loadByName(printOptions->p_template);
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
		qfile.write(qPrintable(grantlee_template));
		qfile.resize(qfile.pos());
		qfile.close();
	}
}
