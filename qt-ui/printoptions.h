#ifndef PRINTOPTIONS_H
#define PRINTOPTIONS_H

#include <QWidget>

#include "ui_printoptions.h"

// should be based on a custom QPrintDialog class
class PrintOptions : public QWidget {
	Q_OBJECT

public:
	explicit PrintOptions(QWidget *parent = 0, struct options *printOpt = 0);
	void setup(struct options *printOpt);

private:
	Ui::PrintOptions ui;
	struct options *printOptions;
	bool hasSetupSlots;

private
slots:
	void radioSixDivesClicked(bool check);
	void radioTwoDivesClicked(bool check);
	void radioTablePrintClicked(bool check);
	void printInColorClicked(bool check);
	void printSelectedClicked(bool check);
	void notesOnTopClicked(bool check);
	void profileOnTopClicked(bool check);
};

#endif // PRINTOPTIONS_H
