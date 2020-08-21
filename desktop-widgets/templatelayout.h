// SPDX-License-Identifier: GPL-2.0
#ifndef TEMPLATELAYOUT_H
#define TEMPLATELAYOUT_H

#include <QStringList>
#include "mainwindow.h"
#include "printoptions.h"
#include "core/statistics.h"
#include "core/qthelper.h"
#include "core/subsurface-qt/diveobjecthelper.h"
#include "core/subsurface-qt/cylinderobjecthelper.h" // TODO: remove once grantlee supports Q_GADGET objects

int getTotalWork(print_options *printOptions);
void find_all_templates();
void set_bundled_templates_as_read_only();
void copy_bundled_templates(QString src, QString dst, QStringList *templateBackupList);

enum token_t {LITERAL, FORSTART, FORSTOP, BLOCKSTART, BLOCKSTOP, IFSTART, IFSTOP, PARSERERROR};

struct token {
	enum token_t type;
	QString contents;
};

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
	QList<token> lexer(QString input);
	void parser(QList<token> tokenList, int &pos, QTextStream &out, QHash<QString, QVariant> options);
	QVariant getValue(QString list, QString property, QVariant option);
	QString translate(QString s, QHash<QString, QVariant> options);



signals:
	void progressUpdated(int value);
};

struct YearInfo {
	stats_t *year;
};

Q_DECLARE_METATYPE(template_options)
Q_DECLARE_METATYPE(print_options)
Q_DECLARE_METATYPE(YearInfo)

#endif
