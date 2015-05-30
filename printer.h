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
	int done;

private slots:
	void templateProgessUpdated(int value);

public:
	Printer(QPrinter *printer);
	void print();

signals:
	void progessUpdated(int value);
};

#endif //PRINTER_H
