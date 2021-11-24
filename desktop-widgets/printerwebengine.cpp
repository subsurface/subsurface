// SPDX-License-Identifier: GPL-2.0
#include "printerwebengine.h"
#include "mainwindow.h"
#include "templatelayout.h"
#include "core/statistics.h"
#include "core/qthelper.h"
#include "core/settings/qPrefDisplay.h"
#include <stdio.h>

#include <algorithm>
#include <QPainter>
#include <QPrinter>
#include <QtWebEngineWidgets>
#include "profile-widget/profilewidget2.h"

extern void exportProfile(const struct dive *dive, const QString filename);

Printer::Printer(QPaintDevice *paintDevice, print_options &printOptions, template_options &templateOptions, PrintMode printMode, bool inPlanner, QWidget *parent = Q_NULLPTR) :
	paintDevice(paintDevice),
	printOptions(printOptions),
	templateOptions(templateOptions),
	printMode(printMode),
	inPlanner(inPlanner),
	done(0)
{
	webView = new QWebEngineView(parent);
	connect(webView, &QWebEngineView::loadFinished, this, &Printer::onLoadFinished);
		if (printMode == PRINT) {
		connect(this, &Printer::profiles_inserted, this, &Printer::printing);
	}
	profiles_missing = true;
}

Printer::~Printer()
{
	delete webView;
}

void Printer::onLoadFinished()
{
	if (profiles_missing)
		webView->page()->runJavaScript("   var profiles = document.getElementsByClassName(\"diveProfile\");\
				       for (let profile of profiles) { \
					  var id = profile.attributes.getNamedItem(\"Id\").value; \
					  var img = document.createElement(\"img\"); \
					  img.src = id + \".png\"; \
					  img.style.height = \"100%\"; \
					  img.style.width = \"100%\"; \
					  profile.appendChild(img); \
					} \
				", [this](const QVariant &v) { emit profiles_inserted(); });
	profiles_missing = false;
	emit(progessUpdated(100));
}

void Printer::printing()
{
	QPrintDialog printDialog(&printer, (QWidget *) nullptr);
	if (printDialog.exec() == QDialog::Accepted) {
		webView->page()->print(&printer, [this](bool ok){ if (ok) emit jobDone(); });
	}
	printDialog.close();
}

//value: ranges from 0 : 100 and shows the progress of the templating engine
void Printer::templateProgessUpdated(int value)
{
	done = value / 5; //template progess if 1/5 of total work
	emit progessUpdated(done);
}

QString Printer::exportHtml()
{
	// Does anybody actually use this? It will not contian profile images!!!
	TemplateLayout t(printOptions, templateOptions);
	connect(&t, SIGNAL(progressUpdated(int)), this, SLOT(templateProgessUpdated(int)));
	QString html;

	if (printOptions.type == print_options::DIVELIST)
		html = t.generate(inPlanner);
	else if (printOptions.type == print_options::STATISTICS )
		html = t.generateStatistics();

	// TODO: write html to file
	return html;
}

void Printer::updateOptions(print_options &poptions, template_options &toptions)
{
	templateOptions = toptions;
	printOptions = poptions;
	profiles_missing = true;
}

QString Printer::writeTmpTemplate(const QString templtext)
{
	QFile fd(printDir.filePath("template.html"));


	fd.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&fd);
	out << templtext;
	fd.close();
	return fd.fileName();
}

void Printer::print()
{
	// we can only print if "PRINT" mode is selected
	int i;
	struct dive *dive;
	QString fn;
	int dives_to_print = 0;
	for_each_dive(i, dive)
		if (dive->selected || !printOptions.print_selected)
			++dives_to_print;

	for_each_dive (i, dive) {
		//TODO check for exporting selected dives only
		if (!dive->selected && printOptions.print_selected)
			continue;
		exportProfile(dive, printDir.filePath(QString("dive_%1.png").arg(dive->id)));
		emit(progessUpdated(done + lrint(i * 80.0 / dives_to_print)));
	}

	profiles_missing = true;
	TemplateLayout t(printOptions, templateOptions);
	connect(&t, SIGNAL(progressUpdated(int)), this, SLOT(templateProgessUpdated(int)));
	int dpi = printer.resolution();
	webView->page()->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
	connect(webView, &QWebEngineView::loadFinished, this, &Printer::onLoadFinished);

	if (printOptions.type == print_options::DIVELIST) {
		QFile printFile(printDir.filePath("print.html"));
		printFile.open(QIODevice::WriteOnly | QIODevice::Text);
		QTextStream out(&printFile);
		out << t.generate(inPlanner);
		printFile.close();
		webView->load(QUrl::fromLocalFile(printDir.filePath("print.html")));
	} else if (printOptions.type == print_options::STATISTICS )
		webView->setHtml(t.generateStatistics());
	if (printOptions.color_selected && printer.colorMode())
		printer.setColorMode(QPrinter::Color);
	else
		printer.setColorMode(QPrinter::GrayScale);
	printer.setResolution(dpi);
}

