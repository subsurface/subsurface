// SPDX-License-Identifier: GPL-2.0
#include <QFileDevice>
#include <QRegularExpression>
#include <list>

#include "templatelayout.h"
#include "core/divelist.h"
#include "core/selection.h"

QList<QString> grantlee_templates, grantlee_statistics_templates;

int getTotalWork(const print_options &printOptions)
{
	if (printOptions.print_selected) {
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

TemplateLayout::TemplateLayout(const print_options &printOptions, const template_options &templateOptions) :
	printOptions(printOptions), templateOptions(templateOptions)
{
}

QString TemplateLayout::generate()
{
	int progress = 0;
	int totalWork = getTotalWork(printOptions);

	QString htmlContent;

	State state;

	struct dive *dive;
	if (in_planner()) {
		state.dives.append(DiveObjectHelperGrantlee(&displayed_dive));
		emit progressUpdated(100.0);
	} else {
		int i;
		for_each_dive (i, dive) {
			//TODO check for exporting selected dives only
			if (!dive->selected && printOptions.print_selected)
				continue;
			state.dives.append(DiveObjectHelperGrantlee(dive));
			progress++;
			emit progressUpdated(lrint(progress * 100.0 / totalWork));
		}
	}

	QString templateContents = readTemplate(printOptions.p_template);

	QList<token> tokens = lexer(templateContents);
	QString buffer;
	QTextStream out(&buffer);
	parser(tokens, 0, tokens.size(), out, state);
	htmlContent = out.readAll();
	return htmlContent;
}

QString TemplateLayout::generateStatistics()
{
	QString htmlContent;

	State state;

	int i = 0;
	stats_summary_auto_free stats;
	calculate_stats_summary(&stats, false);
	while (stats.stats_yearly != NULL && stats.stats_yearly[i].period) {
		state.years.append(&stats.stats_yearly[i]);
		i++;
	}

	QString templateFile = QString("statistics") + QDir::separator() + printOptions.p_template;
	QString templateContents = readTemplate(templateFile);

	QList<token> tokens = lexer(templateContents);
	QString buffer;
	QTextStream out(&buffer);
	parser(tokens, 0, tokens.size(), out, state);
	htmlContent = out.readAll();
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

QString TemplateLayout::translate(QString s, State &state)
{
	QString out;
	int last = 0;
	QRegularExpressionMatch match = var.match(s);
	while (match.hasMatch()) {
		QString obname = match.captured(1);
		QString memname = match.captured(2);
		out +=  s.mid(last, match.capturedStart() - last);
		QString listname = state.types.value(obname, obname);
		QVariant value = getValue(listname, memname, state);
		out += value.toString();
		last = match.capturedEnd();
		match = var.match(s, last);
	}
	out += s.mid(last);
	return out;
}

static QRegularExpression forloop(R"(\s*(\w+)\s+in\s+(\w+))");	// Look for "VAR in LISTNAME"
static QRegularExpression ifstatement(R"(forloop\.counter\|\s*divisibleby\:\s*(\d+))");	// Look for forloop.counter|divisibleby: NUMBER

template<typename V, typename T>
void TemplateLayout::parser_for(QList<token> tokenList, int from, int to, QTextStream &out, State &state,
				const V &data, const T *&act)
{
	const T *old = act;
	int i = 1; // Loop iterators start at one
	int olditerator = state.forloopiterator;
	for (const T &item: data) {
		act = &item;
		state.forloopiterator = i++;
		parser(tokenList, from, to, out, state);
	}
	act = old;
	state.forloopiterator = olditerator;
}

// Find end of for or if block. Keeps track of nested blocks.
// Pos should point one past the starting tag.
// Returns -1 if no matching end tag found.
static int findEnd(const QList<token> &tokenList, int from, int to, token_t start, token_t end)
{
	int depth = 1;
	for (int pos = from; pos < to; ++pos) {
		if (tokenList[pos].type == start) {
			++depth;
		} else if (tokenList[pos].type == end) {
			if (--depth <= 0)
				return pos;
		}
	}
	return -1;
}

void TemplateLayout::parser(QList<token> tokenList, int from, int to, QTextStream &out, State &state)
{
	for (int pos = from; pos < to; ++pos) {
		switch (tokenList[pos].type) {
		case LITERAL:
			out << translate(tokenList[pos].contents, state);
			break;
		case BLOCKSTART:
		case BLOCKSTOP:
			break;
		case FORSTART:
		{
			QString argument = tokenList[pos].contents;
			++pos;
			QRegularExpressionMatch match = forloop.match(argument);
			if (match.hasMatch()) {
				QString itemname = match.captured(1);
				QString listname = match.captured(2);
				state.types[itemname] = listname;
				QString buffer;
				QTextStream capture(&buffer);
				int loop_end = findEnd(tokenList, pos, to, FORSTART, FORSTOP);
				if (loop_end < 0) {
					out << "UNMATCHED FOR: '" << argument << "'";
					break;
				}
				if (listname == "years") {
					parser_for(tokenList, pos, loop_end, capture, state, state.years, state.currentYear);
				} else if (listname == "dives") {
					parser_for(tokenList, pos, loop_end, capture, state, state.dives, state.currentDive);
				} else if (listname == "cylinders") {
					if (state.currentDive)
						parser_for(tokenList, pos, loop_end, capture, state, state.currentDive->cylinders, state.currentCylinder);
					else
						qWarning("cylinders loop outside of dive");
				} else if (listname == "cylinderObjects") {
					if (state.currentDive)
						parser_for(tokenList, pos, loop_end, capture, state, state.currentDive->cylinderObjects, state.currentCylinderObject);
					else
						qWarning("cylinderObjects loop outside of dive");
				} else {
					qWarning("unknown loop: %s", qPrintable(listname));
				}
				state.types.remove(itemname);
				out << capture.readAll();
				pos = loop_end;
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
				int if_end = findEnd(tokenList, pos, to, IFSTART, IFSTOP);
				if (if_end < 0) {
					out << "UNMATCHED IF: '" << argument << "'";
					break;
				}
				int divisor = match.captured(1).toInt();
				int counter = std::max(0, state.forloopiterator);
				if (!(counter % divisor)) {
					QString buffer;
					QTextStream capture(&buffer);
					parser(tokenList, pos, if_end, capture, state);
					out << capture.readAll();
				}
				pos = if_end;
			} else {
				out << "PARSING ERROR: '" << argument << "'";
			}
		}
			break;
		case FORSTOP:
		case IFSTOP:
			out << "UNEXPECTED END: " << tokenList[pos].contents;
			return;
		case PARSERERROR:
			out << "PARSING ERROR";
		}
	}
}

QVariant TemplateLayout::getValue(QString list, QString property, const State &state)
{
	if (list == "template_options") {
		if (property == "font") {
			switch (templateOptions.font_index) {
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
			return templateOptions.border_width;
		} else if (property == "font_size") {
			return templateOptions.font_size / 9.0;
		} else if (property == "line_spacing") {
			return templateOptions.line_spacing;
		} else if (property == "color1") {
			return templateOptions.color_palette.color1.name();
		} else if (property == "color2") {
			return templateOptions.color_palette.color2.name();
		} else if (property == "color3") {
			return templateOptions.color_palette.color3.name();
		} else if (property == "color4") {
			return templateOptions.color_palette.color4.name();
		} else if (property == "color5") {
			return templateOptions.color_palette.color5.name();
		} else if (property == "color6") {
			return templateOptions.color_palette.color6.name();
		}
	} else if (list ==  "print_options") {
		if (property == "grayscale") {
			if (printOptions.color_selected) {
				return "";
			} else {
				return "-webkit-filter: grayscale(100%)";
			}
		}
	} else if (list =="years") {
		if (!state.currentYear)
			return QVariant();
		const stats_t *object = *state.currentYear;
		if (property == "year") {
			return object->period;
		} else if (property == "dives") {
			return object->selection_size;
		} else if (property == "min_temp") {
			return object->min_temp.mkelvin == 0 ? "0" : get_temperature_string(object->min_temp, true);
		} else if (property == "max_temp") {
			return object->max_temp.mkelvin == 0 ? "0" : get_temperature_string(object->max_temp, true);
		} else if (property == "total_time") {
			return get_dive_duration_string(object->total_time.seconds, gettextFromC::tr("h"),
							gettextFromC::tr("min"), gettextFromC::tr("sec"), " ");
		} else if (property == "avg_time") {
			return get_minutes(object->total_time.seconds / object->selection_size);
		} else if (property == "shortest_time") {
			return get_minutes(object->shortest_time.seconds);
		} else if (property == "longest_time") {
			return get_minutes(object->longest_time.seconds);
		} else if (property == "avg_depth") {
			return get_depth_string(object->avg_depth);
		} else if (property == "min_depth") {
			return get_depth_string(object->min_depth);
		} else if (property == "max_depth") {
			return get_depth_string(object->max_depth);
		} else if (property == "avg_sac") {
			return get_volume_string(object->avg_sac);
		} else if (property == "min_sac") {
			return get_volume_string(object->min_sac);
		} else if (property == "max_sac") {
			return get_volume_string(object->max_sac);
		}
	} else if (list == "cylinders") {
		if (state.currentCylinder && property == "description") {
			return *state.currentCylinder;
		}
	} else if (list == "cylinderObjects") {
		if (!state.currentCylinderObject)
			return QVariant();
		const CylinderObjectHelper &object = *state.currentCylinderObject;
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
	} else if (list == "dives") {
		if (!state.currentDive)
			return QVariant();
		const DiveObjectHelperGrantlee &object = *state.currentDive;
		if (property == "number") {
			return object.number;
		} else if (property == "id") {
			return object.id;
		} else if (property == "rating") {
			return object.rating;
		} else if (property == "visibility") {
			return object.visibility;
		} else if (property == "wavesize") {
			return object.wavesize;
		} else if (property == "current") {
			return object.current;
		} else if (property == "surge") {
			return object.surge;
		} else if (property == "chill") {
			return object.chill;
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
