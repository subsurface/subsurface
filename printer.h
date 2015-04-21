#ifndef PRINTER_H
#define PRINTER_H

#include <QPrinter>
#include <QWebView>

class Printer : public QObject {
	Q_OBJECT

private:
	QPrinter *printer;
	QWebView *webView;
	void render();

public:
	Printer(QPrinter *printer);
	void print();
};

#endif //PRINTER_H
