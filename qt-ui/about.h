#ifndef ABOUT_H
#define ABOUT_H

#include <QDialog>
#include <QPushButton>
#include "ui_about.h"

class SubsurfaceAbout : public QDialog {
	Q_OBJECT

public:
	static SubsurfaceAbout* instance();
private slots:
	void licenseClicked();
	void websiteClicked();
private:
	explicit SubsurfaceAbout(QWidget* parent = 0, Qt::WindowFlags f = 0);
	QPushButton *licenseButton;
	QPushButton *websiteButton;
	Ui::SubsurfaceAbout ui;
};

#endif
