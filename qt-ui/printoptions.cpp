#include "printoptions.h"
#include "ui_printoptions.h"

PrintOptions *PrintOptions::instance()
{
	static PrintOptions *self = new PrintOptions();	
	return self;
}

PrintOptions::PrintOptions(QWidget *parent, Qt::WindowFlags f)
: ui( new Ui::PrintOptions())
{
	ui->setupUi(this);
}
