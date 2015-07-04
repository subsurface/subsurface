#ifndef PRINTOPTIONS_H
#define PRINTOPTIONS_H

#include <QWidget>

#include "ui_printoptions.h"

struct print_options {
	enum print_type {
		DIVELIST,
		TABLE,
		STATISTICS
	} type;
	enum print_template {
		ONE_DIVE,
		TWO_DIVE,
		CUSTOM
	} p_template;
	bool print_selected;
	bool color_selected;
	bool landscape;
};

// should be based on a custom QPrintDialog class
class PrintOptions : public QWidget {
	Q_OBJECT

public:
	explicit PrintOptions(QWidget *parent, struct print_options *printOpt, struct template_options *templateOpt);
	void setup(struct print_options *printOpt);

private:
	Ui::PrintOptions ui;
	struct print_options *printOptions;
	struct template_options *templateOptions;
	bool hasSetupSlots;

private
slots:
	void printInColorClicked(bool check);
	void printSelectedClicked(bool check);
	void on_radioStatisticsPrint_clicked(bool check);
	void on_radioTablePrint_clicked(bool check);
	void on_radioDiveListPrint_clicked(bool check);
	void on_printTemplate_currentIndexChanged(int index);
	void on_editButton_clicked();
};

#endif // PRINTOPTIONS_H
