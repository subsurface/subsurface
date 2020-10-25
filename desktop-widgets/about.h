// SPDX-License-Identifier: GPL-2.0
#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>
#include "ui_about.h"

class SubsurfaceAbout : public QDialog {
	Q_OBJECT

public:
	explicit SubsurfaceAbout(QWidget *parent = 0);
private
slots:
	void on_licenseButton_clicked();
	void on_websiteButton_clicked();
	void on_creditButton_clicked();

private:
	Ui::SubsurfaceAbout ui;
};

#endif // ABOUT_H
