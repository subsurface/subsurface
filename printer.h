#ifndef PRINTER_H
#define PRINTER_H

#include <QPrinter>
#include <QWebView>
#include <QRect>
#include <QPainter>

#include "profile/profilewidget2.h"
#include "printoptions.h"

class Printer : public QObject {
	Q_OBJECT

private:
	QPrinter *printer;
	QWebView *webView;
	print_options *printOptions;
	QSize pageSize;
	int done;
	int dpi;
	void render();
	void putProfileImage(QRect box, QRect viewPort, QPainter *painter, struct dive *dive, QPointer<ProfileWidget2> profile);

private slots:
	void templateProgessUpdated(int value);

public:
	Printer(QPrinter *printer, print_options *printOptions);
	void print();

signals:
	void progessUpdated(int value);
};

#endif //PRINTER_H
