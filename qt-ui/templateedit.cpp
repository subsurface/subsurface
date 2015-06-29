#include "templateedit.h"
#include "ui_templateedit.h"

TemplateEdit::TemplateEdit(QWidget *parent, struct template_options *templateOptions) :
	QDialog(parent),
	ui(new Ui::TemplateEdit)
{
	ui->setupUi(this);
	this->templateOptions = templateOptions;

	// restore the settings and init the UI
	ui->fontSelection->setCurrentIndex(templateOptions->font_index);
	ui->fontsize->setValue(templateOptions->font_size);
	ui->colorpalette->setCurrentIndex(templateOptions->color_palette_index);
	ui->linespacing->setValue(templateOptions->line_spacing);
}

TemplateEdit::~TemplateEdit()
{
	delete ui;
}

void TemplateEdit::on_fontsize_valueChanged(int font_size)
{
	templateOptions->font_size = font_size;
}

void TemplateEdit::on_linespacing_valueChanged(double line_spacing)
{
	templateOptions->line_spacing = line_spacing;
}

void TemplateEdit::on_fontSelection_currentIndexChanged(int index)
{
	templateOptions->font_index = index;
}

void TemplateEdit::on_colorpalette_currentIndexChanged(int index)
{
	templateOptions->color_palette_index = index;
}
