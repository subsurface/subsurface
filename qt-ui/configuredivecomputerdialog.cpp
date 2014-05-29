#include "configuredivecomputerdialog.h"
#include "ui_configuredivecomputerdialog.h"

ConfigureDiveComputerDialog::ConfigureDiveComputerDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConfigureDiveComputerDialog)
{
	ui->setupUi(this);
}

ConfigureDiveComputerDialog::~ConfigureDiveComputerDialog()
{
	delete ui;
}
