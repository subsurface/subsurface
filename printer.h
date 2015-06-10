#ifndef PRINTER_H
#define PRINTER_H

#include <QPrinter>
#include <QWebView>
#include <QRect>
#include <QPainter>

#include "profile/profilewidget2.h"

class Printer : public QObject {
	Q_OBJECT

private:
	QPrinter *printer;
	QWebView *webView;
	void render();
	void putProfileImage(QRect box, QRect viewPort, QPainter *painter, struct dive *dive, QPointer<ProfileWidget2> profile);
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
