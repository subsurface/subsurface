// SPDX-License-Identifier: GPL-2.0
#include "printerwebengine.h"
#include "mainwindow.h"
#include "templatelayout.h"
#include "core/statistics.h"
#include "core/qthelper.h"
#include "core/settings/qPrefDisplay.h"
#include "profile-widget/profilewidget2.h"

#include <algorithm>
#include <QPainter>
#include <QPrinter>
#include <QtWebEngineWidgets>

extern void exportProfile(const struct dive *dive, const QString filename);

Printer::Printer(QPaintDevice *paintDevice, print_options &printOptions, template_options &templateOptions, PrintMode printMode, bool inPlanner, QWidget *parent = nullptr) :
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
		connect(this, &Printer::profilesInserted, this, &Printer::reopen);
		connect(this, &Printer::jobDone, this, &Printer::printFinished);
	}
	profilesMissing = true;
}

Printer::~Printer()
{
	qDebug() << "deleting Printer object";
	delete webView;
}

void Printer::onLoadFinished()
{
	qDebug() << "onLoadFinished called with profilesMissing" << profilesMissing << "and URL" << webView->url().toString();
	if (profilesMissing) {
		QString jsText("   var profiles = document.getElementsByClassName(\"diveProfile\");\
			       for (let profile of profiles) { \
				  var id = profile.attributes.getNamedItem(\"Id\").value; \
				  var img = document.createElement(\"img\"); \
				  img.src = \"TMPPATH\" + id + \".png\"; \
				  img.style.height = \"100%\"; \
				  img.style.width = \"100%\"; \
				  profile.appendChild(img); \
				} \
				document.documentElement.innerHTML; \
			");
		QString filePath = QStringLiteral("file://") + printDir.path() + QDir::separator();
		jsText.replace("TMPPATH", filePath);
		webView->page()->runJavaScript(jsText, [this](const QVariant &v) {
					qDebug() << "JS finished";
					QFile fd(printDir.filePath("finalprint.html"));
					fd.open(QIODevice::WriteOnly | QIODevice::Text);
					QTextStream out(&fd);
					out << v.toString();
					fd.close();
					qDebug() << "finalprint.html written";
					profilesMissing = false;
					emit(progessUpdated(80));
					emit profilesInserted();
				});
	} else {
		qDebug() << "load of" << webView->url().toString() << "finished - open printing dialog";
		emit(progessUpdated(100));
		printing();
	}
}

// the Javascript code injecting the profiles has completed, let's reload that page
void Printer::reopen()
{
	qDebug() << "opening the finalprint.html file";
	webView->load(QUrl::fromLocalFile(printDir.filePath("finalprint.html")));
}

void Printer::printing()
{
	QPrintDialog printDialog(&printer, (QWidget *) nullptr);
	if (printDialog.exec() == QDialog::Accepted)
		webView->page()->print(&printer, [this](bool ok){
			if (ok)
				emit jobDone();
			qDebug() << "printing done with status " << ok ;
			qDebug() << "content of" << printDir.path();
			qDebug() << QDir(printDir.path()).entryList();
		});
	qDebug() << "closing the dialog now";
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
	profilesMissing = true;
}

QString Printer::writeTmpTemplate(const QString templtext)
{
	QFile fd(printDir.filePath("ssrftmptemplate.html"));

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

	profilesMissing = true;
	TemplateLayout t(printOptions, templateOptions);
	connect(&t, SIGNAL(progressUpdated(int)), this, SLOT(templateProgessUpdated(int)));
	int dpi = printer.resolution();
	webView->page()->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);

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

void Printer::printFinished()
{
	qDebug() << "received the jobDone signal, closing the dialog; page should have referenced:";
	webView->page()->toHtml([](const QString h){
		QRegularExpression img("img src=([^ ]+) ");
		QRegularExpressionMatchIterator iter = img.globalMatch(h);
		while(iter.hasNext()) {
			QRegularExpressionMatch match = iter.next();
			QString filename = match.captured(1).replace("file://", "");
			qDebug() << filename << (QFile(filename).exists() ? "which does" : "which doesn't") << "exist";
		}
	});

}
