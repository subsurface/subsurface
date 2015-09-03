#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>
#include "ui_about.h"

class SubsurfaceAbout : public QDialog {
	Q_OBJECT

public:
	explicit SubsurfaceAbout(QWidget *parent = 0, Qt::WindowFlags f = 0);
private
slots:
	void on_licenseButton_clicked();
	void on_websiteButton_clicked();

private:
	Ui::SubsurfaceAbout ui;
};

#endif // ABOUT_H
