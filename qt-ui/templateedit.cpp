#include "templateedit.h"
#include "ui_templateedit.h"

TemplateEdit::TemplateEdit(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::TemplateEdit)
{
	ui->setupUi(this);
	this->templateOptions = templateOptions;
}

TemplateEdit::~TemplateEdit()
{
	delete ui;
}
