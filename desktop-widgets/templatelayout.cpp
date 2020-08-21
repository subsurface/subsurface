// SPDX-License-Identifier: GPL-2.0
#include <QFileDevice>
#include <QRegularExpression>
#include <list>

#include "templatelayout.h"
#include "core/divelist.h"
#include "core/selection.h"

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

	QHash<QString, QVariant> options;
	options["print_options"] = QVariant::fromValue(*printOptions);
	options["template_options"] = QVariant::fromValue(*templateOptions);
	options["dives"] = QVariant::fromValue(diveList);
	QList<token> tokens = lexer(templateContents);
	QString buffer;
	QTextStream out(&buffer);
	int pos = 0;
	parser(tokens, pos, out, options);
	htmlContent = out.readAll();
	return htmlContent;

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
	QString templateFile = QString("statistics") + QDir::separator() + printOptions->p_template;
	QString templateContents = readTemplate(templateFile);

	QHash<QString, QVariant> options;
	options["print_options"] = QVariant::fromValue(*printOptions);
	options["template_options"] = QVariant::fromValue(*templateOptions);
	options["years"] = QVariant::fromValue(years);
	QList<token> tokens = lexer(templateContents);
	QString buffer;
	QTextStream out(&buffer);
	int pos = 0;
	parser(tokens, pos, out, options);
	htmlContent = out.readAll();
	return htmlContent;


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

struct token stringToken(QString s)
{
	struct token newtoken;
	newtoken.type = LITERAL;
	newtoken.contents = s;
	return newtoken;
}

static QRegularExpression keywordFor(R"(\bfor\b)");
static QRegularExpression keywordEndfor(R"(\bendfor\b)");
static QRegularExpression keywordBlock(R"(\bblock\b)");
static QRegularExpression keywordEndblock(R"(\bendblock\b)");
static QRegularExpression keywordIf(R"(\bif\b)");
static QRegularExpression keywordEndif(R"(\bendif\b)");

struct token operatorToken(QString s)
{
	struct token newtoken;

	QRegularExpressionMatch match = keywordFor.match(s);
	if (match.hasMatch()) {
		newtoken.type = FORSTART;
		newtoken.contents = s.mid(match.capturedEnd());
		return newtoken;
	}
	match = keywordEndfor.match(s);
	if (match.hasMatch()) {
		newtoken.type = FORSTOP;
		newtoken.contents = "";
		return newtoken;
	}
	match = keywordBlock.match(s);
	if (match.hasMatch()) {
		newtoken.type = BLOCKSTART;
		newtoken.contents = s.mid(match.capturedEnd());
		return newtoken;
	}
	match = keywordEndblock.match(s);
	if (match.hasMatch()) {
		newtoken.type = BLOCKSTOP;
		newtoken.contents = "";
		return newtoken;
	}
	match = keywordIf.match(s);
	if (match.hasMatch()) {
		newtoken.type = IFSTART;
		newtoken.contents = s.mid(match.capturedEnd());
		return newtoken;
	}
	match = keywordEndif.match(s);
	if (match.hasMatch()) {
		newtoken.type = IFSTOP;
		newtoken.contents = "";
		return newtoken;
	}

	newtoken.type = PARSERERROR;
	newtoken.contents = "";
	return newtoken;
}

static QRegularExpression op(R"(\{%([\w\s\.\|\:]+)%\})");	// Look for {% stuff %}

QList<token> TemplateLayout::lexer(QString input)
{
	QList<token> tokenList;

	int last = 0;
	QRegularExpressionMatch match = op.match(input);
	while (match.hasMatch()) {
		tokenList << stringToken(input.mid(last, match.capturedStart() - last));
		tokenList << operatorToken(match.captured(1));
		last = match.capturedEnd();
		match = op.match(input, last);
	}
	tokenList << stringToken(input.mid(last));
	return tokenList;
}

static QRegularExpression var(R"(\{\{\s*(\w+)\.(\w+)\s*(\|\s*(\w+))?\s*\}\})");	// Look for {{ stuff.stuff|stuff }}

QString TemplateLayout::translate(QString s, QHash<QString, QVariant> options)
{
	QString out;
	int last = 0;
	QRegularExpressionMatch match = var.match(s);
	while (match.hasMatch()) {
		QString obname = match.captured(1);
		QString memname = match.captured(2);
		out +=  s.mid(last, match.capturedStart() - last);
		QVariant value = getValue(obname, memname, options.value(obname));
		out += value.toString();
		last = match.capturedEnd();
		match = var.match(s, last);
	}
	out += s.mid(last);
	return out;
}

static QRegularExpression forloop(R"(\s*(\w+)\s+in\s+(\w+))");	// Look for "VAR in LISTNAME"
static QRegularExpression ifstatement(R"(forloop\.counter\|\s*divisibleby\:\s*(\d+))");	// Look for forloop.counter|divisibleby: NUMBER

void TemplateLayout::parser(QList<token> tokenList, int &pos, QTextStream &out, QHash<QString, QVariant> options)
{
	while (pos < tokenList.length()) {
		switch (tokenList[pos].type) {
		case LITERAL:
			out << translate(tokenList[pos].contents, options);
			++pos;
			break;
		case BLOCKSTART:
		case BLOCKSTOP:
			++pos;
			break;
		case FORSTART:
		{
			QString argument = tokenList[pos].contents;
			++pos;
			QRegularExpressionMatch match = forloop.match(argument);
			if (match.hasMatch()) {
				QString itemname = match.captured(1);
				QString listname = match.captured(2);
				QString buffer;
				QTextStream capture(&buffer);
				QVariantList list = options[listname].value<QVariantList>();
				int savepos = pos;
				for (int i = 0; i < list.size(); ++i) {
					QVariant item = list.at(i);
					QVariant olditerator = options["forloopiterator"];
					options[itemname] = item;
					options["forloopiterator"] = i + 1;
					pos = savepos;
					if (listname == "dives")
						options["cylinders"] = QVariant::fromValue(item.value<DiveObjectHelperGrantlee>().cylinderList());
					parser(tokenList, pos, capture, options);
					options.remove(itemname);
					options.remove("forloopiterator");
					if (listname == "dives")
						options.remove("cylinders");
					if (olditerator.isValid())
						options["forloopiterator"] = olditerator;
				}
				out << capture.readAll();
			} else {
				out << "PARSING ERROR: '" << argument << "'";
			}
		}
			break;
		case IFSTART:
		{
			QString argument = tokenList[pos].contents;
			++pos;
			QRegularExpressionMatch match = ifstatement.match(argument);
			if (match.hasMatch()) {
				int divisor = match.captured(1).toInt();
				QString buffer;
				QTextStream capture(&buffer);
				int counter = options["forloopiterator"].toInt();
				parser(tokenList, pos, capture, options);
				if (!(counter  % divisor))
					out << capture.readAll();
			} else {
				out << "PARSING ERROR: '" << argument << "'";
			}
		}
			break;
		case FORSTOP:
		case IFSTOP:
			++pos;
			return;
		case PARSERERROR:
			out << "PARSING ERROR";
			++pos;
		}
	}
}

QVariant TemplateLayout::getValue(QString list, QString property, QVariant option)
{
	if (list == "template_options") {
		template_options object = option.value<template_options>();
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
	} else if (list ==  "print_options") {
		print_options object = option.value<print_options>();
		if (property == "grayscale") {
			if (object.color_selected) {
				return "";
			} else {
				return "-webkit-filter: grayscale(100%)";
			}
		}
	} else if (list =="year") {
		YearInfo object = option.value<YearInfo>();
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
	} else if (list == "cylinder") {
		CylinderObjectHelper object = option.value<CylinderObjectHelper>();
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
	} else if (list == "dive") {
		DiveObjectHelperGrantlee object = option.value<DiveObjectHelperGrantlee>();
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
	}
	return QVariant();
}
