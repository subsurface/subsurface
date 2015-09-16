#include "templateedit.h"
#include "printoptions.h"
#include "printer.h"
#include "ui_templateedit.h"

#include <QMessageBox>
#include <QColorDialog>

TemplateEdit::TemplateEdit(QWidget *parent, struct print_options *printOptions, struct template_options *templateOptions) :
	QDialog(parent),
	ui(new Ui::TemplateEdit)
{
	ui->setupUi(this);
	this->templateOptions = templateOptions;
	newTemplateOptions = *templateOptions;
	this->printOptions = printOptions;

	// restore the settings and init the UI
	ui->fontSelection->setCurrentIndex(templateOptions->font_index);
	ui->fontsize->setValue(templateOptions->font_size);
	ui->colorpalette->setCurrentIndex(templateOptions->color_palette_index);
	ui->linespacing->setValue(templateOptions->line_spacing);

	grantlee_template = TemplateLayout::readTemplate(printOptions->p_template);
	if (printOptions->type == print_options::DIVELIST)
		grantlee_template = TemplateLayout::readTemplate(printOptions->p_template);
	else if (printOptions->type == print_options::STATISTICS)
		grantlee_template = TemplateLayout::readTemplate(QString::fromUtf8("statistics") + QDir::separator() + printOptions->p_template);

	// gui
	btnGroup = new QButtonGroup;
	btnGroup->addButton(ui->editButton1, 1);
	btnGroup->addButton(ui->editButton2, 2);
	btnGroup->addButton(ui->editButton3, 3);
	btnGroup->addButton(ui->editButton4, 4);
	btnGroup->addButton(ui->editButton5, 5);
	btnGroup->addButton(ui->editButton6, 6);
	connect(btnGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(colorSelect(QAbstractButton*)));

	ui->plainTextEdit->setPlainText(grantlee_template);
	editingCustomColors = false;
	updatePreview();
}

TemplateEdit::~TemplateEdit()
{
	delete btnGroup;
	delete ui;
}

void TemplateEdit::updatePreview()
{
	// update Qpixmap preview
	int width = ui->label->width();
	int height = ui->label->height();
	QPixmap map(width * 2, height * 2);
	map.fill(QColor::fromRgb(255, 255, 255));
	Printer printer(&map, printOptions, &newTemplateOptions, Printer::PREVIEW);
	printer.previewOnePage();
	ui->label->setPixmap(map.scaled(width, height, Qt::IgnoreAspectRatio));

	// update colors tab
	ui->colorLable1->setStyleSheet("QLabel { background-color : \"" + newTemplateOptions.color_palette.color1.name() + "\";}");
	ui->colorLable2->setStyleSheet("QLabel { background-color : \"" + newTemplateOptions.color_palette.color2.name() + "\";}");
	ui->colorLable3->setStyleSheet("QLabel { background-color : \"" + newTemplateOptions.color_palette.color3.name() + "\";}");
	ui->colorLable4->setStyleSheet("QLabel { background-color : \"" + newTemplateOptions.color_palette.color4.name() + "\";}");
	ui->colorLable5->setStyleSheet("QLabel { background-color : \"" + newTemplateOptions.color_palette.color5.name() + "\";}");
	ui->colorLable6->setStyleSheet("QLabel { background-color : \"" + newTemplateOptions.color_palette.color6.name() + "\";}");

	ui->colorLable1->setText(newTemplateOptions.color_palette.color1.name());
	ui->colorLable2->setText(newTemplateOptions.color_palette.color2.name());
	ui->colorLable3->setText(newTemplateOptions.color_palette.color3.name());
	ui->colorLable4->setText(newTemplateOptions.color_palette.color4.name());
	ui->colorLable5->setText(newTemplateOptions.color_palette.color5.name());
	ui->colorLable6->setText(newTemplateOptions.color_palette.color6.name());

	// update critical UI elements
	ui->colorpalette->setCurrentIndex(newTemplateOptions.color_palette_index);

	// update grantlee template string
	grantlee_template = TemplateLayout::readTemplate(printOptions->p_template);
	if (printOptions->type == print_options::DIVELIST)
		grantlee_template = TemplateLayout::readTemplate(printOptions->p_template);
	else if (printOptions->type == print_options::STATISTICS)
		grantlee_template = TemplateLayout::readTemplate(QString::fromUtf8("statistics") + QDir::separator() + printOptions->p_template);
}

void TemplateEdit::on_fontsize_valueChanged(int font_size)
{
	newTemplateOptions.font_size = font_size;
	updatePreview();
}

void TemplateEdit::on_linespacing_valueChanged(double line_spacing)
{
	newTemplateOptions.line_spacing = line_spacing;
	updatePreview();
}

void TemplateEdit::on_fontSelection_currentIndexChanged(int index)
{
	newTemplateOptions.font_index = index;
	updatePreview();
}

void TemplateEdit::on_colorpalette_currentIndexChanged(int index)
{
	newTemplateOptions.color_palette_index = index;
	switch (newTemplateOptions.color_palette_index) {
	case SSRF_COLORS: // subsurface derived default colors
		newTemplateOptions.color_palette = ssrf_colors;
		break;
	case ALMOND: // almond
		newTemplateOptions.color_palette = almond_colors;
		break;
	case BLUESHADES: // blueshades
		newTemplateOptions.color_palette = blueshades_colors;
		break;
	case CUSTOM: // custom
		if (!editingCustomColors)
			newTemplateOptions.color_palette = custom_colors;
		else
			editingCustomColors = false;
		break;
	}
	updatePreview();
}

void TemplateEdit::saveSettings()
{
	QStringList bundledTemplates;
	bundledTemplates << "Flowlayout.html" << "One Dive.html" << "Six Dives.html" << "Table.html" << "Two Dives.html";
	if ((*templateOptions) != newTemplateOptions || grantlee_template.compare(ui->plainTextEdit->toPlainText())) {
		QMessageBox msgBox(this);
		QString message = tr("Do you want to save your changes?");
		bool templateChanged = false;
		if (grantlee_template.compare(ui->plainTextEdit->toPlainText())) {
			if (bundledTemplates.contains(printOptions->p_template) || (printOptions->p_template == "Default.html" && printOptions->type == print_options::STATISTICS)) {
				msgBox.setIcon(QMessageBox::Warning);
				message = tr("You are about to modify a template bundled with Subsurface.\n") + message;
			}
			templateChanged = true;
		}
		msgBox.setText(message);
		msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
		msgBox.setDefaultButton(QMessageBox::Cancel);
		if (msgBox.exec() == QMessageBox::Save) {
			memcpy(templateOptions, &newTemplateOptions, sizeof(struct template_options));
			if (templateChanged) {
				TemplateLayout::writeTemplate(printOptions->p_template, ui->plainTextEdit->toPlainText());
				if (printOptions->type == print_options::DIVELIST)
					TemplateLayout::writeTemplate(printOptions->p_template, ui->plainTextEdit->toPlainText());
				else if (printOptions->type == print_options::STATISTICS)
					TemplateLayout::writeTemplate(QString::fromUtf8("statistics") + QDir::separator() + printOptions->p_template, ui->plainTextEdit->toPlainText());
			}
			if (templateOptions->color_palette_index == CUSTOM)
				custom_colors = templateOptions->color_palette;
		}
	}
}

void TemplateEdit::on_buttonBox_clicked(QAbstractButton *button)
{
	QDialogButtonBox::StandardButton standardButton = ui->buttonBox->standardButton(button);
	switch (standardButton) {
	case QDialogButtonBox::Ok:
		saveSettings();
		break;
	case QDialogButtonBox::Cancel:
		break;
	case QDialogButtonBox::Apply:
		saveSettings();
		updatePreview();
		break;
	default:
		;
	}
}

void TemplateEdit::colorSelect(QAbstractButton *button)
{
	editingCustomColors = true;
	// reset custom colors palette
	switch (newTemplateOptions.color_palette_index) {
	case SSRF_COLORS: // subsurface derived default colors
		newTemplateOptions.color_palette = ssrf_colors;
		break;
	case ALMOND: // almond
		newTemplateOptions.color_palette = almond_colors;
		break;
	case BLUESHADES: // blueshades
		newTemplateOptions.color_palette = blueshades_colors;
		break;
	default:
		break;
	}

	//change selected color
	QColor color;
	switch (btnGroup->id(button)) {
	case 1:
		color = QColorDialog::getColor(newTemplateOptions.color_palette.color1, this);
		if (color.isValid())
			newTemplateOptions.color_palette.color1 = color;
		break;
	case 2:
		color = QColorDialog::getColor(newTemplateOptions.color_palette.color2, this);
		if (color.isValid())
			newTemplateOptions.color_palette.color2 = color;
		break;
	case 3:
		color = QColorDialog::getColor(newTemplateOptions.color_palette.color3, this);
		if (color.isValid())
			newTemplateOptions.color_palette.color3 = color;
		break;
	case 4:
		color = QColorDialog::getColor(newTemplateOptions.color_palette.color4, this);
		if (color.isValid())
			newTemplateOptions.color_palette.color4 = color;
		break;
	case 5:
		color = QColorDialog::getColor(newTemplateOptions.color_palette.color5, this);
		if (color.isValid())
			newTemplateOptions.color_palette.color5 = color;
		break;
	case 6:
		color = QColorDialog::getColor(newTemplateOptions.color_palette.color6, this);
		if (color.isValid())
			newTemplateOptions.color_palette.color6 = color;
		break;
	}
	newTemplateOptions.color_palette_index = CUSTOM;
	updatePreview();
}
