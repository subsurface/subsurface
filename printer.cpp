#include "printer.h"
#include "templatelayout.h"

#include <QtWebKitWidgets>
#include <QPainter>

#define A4_300DPI_WIDTH 2480
#define A4_300DPI_HIGHT 3508

Printer::Printer(QPrinter *printer)
{
	this->printer = printer;

	//override these settings for now.
	printer->setFullPage(true);
	printer->setOrientation(QPrinter::Portrait);
	printer->setPaperSize(QPrinter::A4);
	printer->setPrintRange(QPrinter::AllPages);
	printer->setResolution(300);
}

void Printer::render()
{
	QPainter painter;
	QSize size(A4_300DPI_WIDTH, A4_300DPI_HIGHT);
	painter.begin(printer);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);

	webView->page()->setViewportSize(size);

	int Pages = ceil((float)webView->page()->mainFrame()->contentsSize().rheight() / A4_300DPI_HIGHT);
	for (int i = 0; i < Pages; i++) {
		webView->page()->mainFrame()->render(&painter, QWebFrame::ContentsLayer);
		webView->page()->mainFrame()->scroll(0, A4_300DPI_HIGHT);
		if (i < Pages - 1)
			printer->newPage();
	}
	painter.end();
}

void Printer::print()
{
	TemplateLayout t;
	webView = new QWebView();
	webView->setHtml(t.generate());
	render();
}
