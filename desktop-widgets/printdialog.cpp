// SPDX-License-Identifier: GPL-2.0
#include "printdialog.h"
#include "printer.h"
#include "core/pref.h"
#include "core/dive.h" // for existing_filename

#ifndef NO_PRINTING
#include <QProgressBar>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QPrintDialog>
#include <QFileDialog>
#include <QShortcut>
#include <QSettings>
#include <QMessageBox>
#include <QDialogButtonBox>

#define SETTINGS_GROUP "PrintDialog"

template_options::color_palette_struct ssrf_colors, almond_colors, blueshades_colors, custom_colors;

PrintDialog::PrintDialog(bool inPlanner, QWidget *parent) :
	QDialog(parent, QFlag(0)),
	inPlanner(inPlanner),
	printer(NULL),
	qprinter(NULL)
{
	// initialize const colors
	ssrf_colors.color1 = QColor::fromRgb(0xff, 0xff, 0xff);
	ssrf_colors.color2 = QColor::fromRgb(0xa6, 0xbc, 0xd7);
	ssrf_colors.color3 = QColor::fromRgb(0xef, 0xf7, 0xff);
	ssrf_colors.color4 = QColor::fromRgb(0x34, 0x65, 0xa4);
	ssrf_colors.color5 = QColor::fromRgb(0x20, 0x4a, 0x87);
	ssrf_colors.color6 = QColor::fromRgb(0x17, 0x37, 0x64);
	almond_colors.color1 = QColor::fromRgb(255, 255, 255);
	almond_colors.color2 = QColor::fromRgb(253, 204, 156);
	almond_colors.color3 = QColor::fromRgb(243, 234, 207);
	almond_colors.color4 = QColor::fromRgb(136, 160, 150);
	almond_colors.color5 = QColor::fromRgb(187, 171, 139);
	almond_colors.color6 = QColor::fromRgb(0, 0, 0);
	blueshades_colors.color1 = QColor::fromRgb(255, 255, 255);
	blueshades_colors.color2 = QColor::fromRgb(142, 152, 166);
	blueshades_colors.color3 = QColor::fromRgb(182, 192, 206);
	blueshades_colors.color4 = QColor::fromRgb(31, 49, 75);
	blueshades_colors.color5 = QColor::fromRgb(21, 45, 84);
	blueshades_colors.color6 = QColor::fromRgb(0, 0, 0);

	// check if the options were previously stored in the settings; if not use some defaults.
	QSettings s;
	s.beginGroup(SETTINGS_GROUP);
	printOptions.type = (print_options::print_type)s.value("type", print_options::DIVELIST).toInt();
	printOptions.print_selected = s.value("print_selected", true).toBool();
	printOptions.color_selected = s.value("color_selected", true).toBool();
	printOptions.landscape = s.value("landscape", false).toBool();
	printOptions.p_template = s.value("template_selected", "one_dive.html").toString();
	printOptions.resolution = s.value("resolution", 600).toInt();
	templateOptions.font_index = s.value("font", 0).toInt();
	templateOptions.font_size = s.value("font_size", 9).toDouble();
	templateOptions.color_palette_index = s.value("color_palette", SSRF_COLORS).toInt();
	templateOptions.line_spacing = s.value("line_spacing", 1).toDouble();
	templateOptions.border_width = s.value("border_width", 1).toInt();
	custom_colors.color1 = QColor(s.value("custom_color_1", ssrf_colors.color1).toString());
	custom_colors.color2 = QColor(s.value("custom_color_2", ssrf_colors.color2).toString());
	custom_colors.color3 = QColor(s.value("custom_color_3", ssrf_colors.color3).toString());
	custom_colors.color4 = QColor(s.value("custom_color_4", ssrf_colors.color4).toString());
	custom_colors.color5 = QColor(s.value("custom_color_5", ssrf_colors.color5).toString());
	s.endGroup();

	// handle cases from old QSettings group
	if (templateOptions.font_size < 9) {
		templateOptions.font_size = 9;
	}
	if (templateOptions.line_spacing < 1) {
		templateOptions.line_spacing = 1;
	}

	switch (templateOptions.color_palette_index) {
	case SSRF_COLORS: // default Subsurface derived colors
		templateOptions.color_palette = ssrf_colors;
		break;
	case ALMOND: // almond
		templateOptions.color_palette = almond_colors;
		break;
	case BLUESHADES: // blueshades
		templateOptions.color_palette = blueshades_colors;
		break;
	case CUSTOM: // custom
		templateOptions.color_palette = custom_colors;
		break;
	}

	// create a print options object and pass our options struct
	optionsWidget = new PrintOptions(this, printOptions, templateOptions);

	QVBoxLayout *layout = new QVBoxLayout(this);
	setLayout(layout);

	layout->addWidget(optionsWidget);

	progressBar = new QProgressBar();
	progressBar->setMinimum(0);
	progressBar->setMaximum(100);
	progressBar->setValue(0);
	progressBar->setTextVisible(false);
	layout->addWidget(progressBar);

	QHBoxLayout *hLayout = new QHBoxLayout();
	layout->addLayout(hLayout);

	QPushButton *printButton = new QPushButton(tr("P&rint"));
	connect(printButton, SIGNAL(clicked(bool)), this, SLOT(printClicked()));

	QPushButton *previewButton = new QPushButton(tr("&Preview"));
	connect(previewButton, SIGNAL(clicked(bool)), this, SLOT(previewClicked()));

	QPushButton *exportHtmlButton = new QPushButton(tr("Export Html"));
	connect(exportHtmlButton, SIGNAL(clicked(bool)), this, SLOT(exportHtmlClicked()));

	QDialogButtonBox *buttonBox = new QDialogButtonBox;
	buttonBox->addButton(QDialogButtonBox::Cancel);
	buttonBox->addButton(printButton, QDialogButtonBox::AcceptRole);
	buttonBox->addButton(previewButton, QDialogButtonBox::ActionRole);
	buttonBox->addButton(exportHtmlButton, QDialogButtonBox::AcceptRole);

	connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

	hLayout->addWidget(buttonBox);

	setWindowTitle(tr("Print"));
	setWindowIcon(QIcon(":subsurface-icon"));

	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));

	// seems to be the most reliable way to track for all sorts of dialog disposal.
	connect(this, SIGNAL(finished(int)), this, SLOT(onFinished()));
}

PrintDialog::~PrintDialog()
{
	delete qprinter;
	delete printer;
}

void PrintDialog::onFinished()
{
	QSettings s;
	s.beginGroup(SETTINGS_GROUP);

	// save print paper settings
	s.setValue("type", printOptions.type);
	s.setValue("print_selected", printOptions.print_selected);
	s.setValue("color_selected", printOptions.color_selected);
	s.setValue("template_selected", printOptions.p_template);
	s.setValue("resolution", printOptions.resolution);

	// save template settings
	s.setValue("font", templateOptions.font_index);
	s.setValue("font_size", templateOptions.font_size);
	s.setValue("color_palette", templateOptions.color_palette_index);
	s.setValue("line_spacing", templateOptions.line_spacing);
	s.setValue("border_width", templateOptions.border_width);

	// save custom colors
	s.setValue("custom_color_1", custom_colors.color1.name());
	s.setValue("custom_color_2", custom_colors.color2.name());
	s.setValue("custom_color_3", custom_colors.color3.name());
	s.setValue("custom_color_4", custom_colors.color4.name());
	s.setValue("custom_color_5", custom_colors.color5.name());
}

void PrintDialog::createPrinterObj()
{
	// create a new printer object
	if (!printer) {
		qprinter = new QPrinter;
		qprinter->setResolution(printOptions.resolution);
		qprinter->setOrientation((QPrinter::Orientation)printOptions.landscape);
		printer = new Printer(qprinter, printOptions, templateOptions, Printer::PRINT, inPlanner);
	}
}

void PrintDialog::previewClicked(void)
{
	createPrinterObj();
	QPrintPreviewDialog previewDialog(qprinter, this, Qt::Window
		| Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint
		| Qt::WindowTitleHint);
	connect(&previewDialog, SIGNAL(paintRequested(QPrinter *)), this, SLOT(onPaintRequested(QPrinter *)));
	previewDialog.exec();
}

void PrintDialog::exportHtmlClicked(void)
{
	createPrinterObj();
	QString saveFileName = printOptions.p_template;
	QString filename = existing_filename ?: prefs.default_filename;
	QFileInfo fi(filename);
	filename = fi.absolutePath().append(QDir::separator()).append(saveFileName);
	QString htmlExportFilename = QFileDialog::getSaveFileName(this, tr("Filename to export html to"),
							  filename, tr("Html file") + " (*.html)");
	if (!htmlExportFilename.isEmpty()) {
		QFile file(htmlExportFilename);
		file.open(QIODevice::WriteOnly);
		connect(printer, SIGNAL(progessUpdated(int)), progressBar, SLOT(setValue(int)));
		file.write(printer->exportHtml().toUtf8());
		file.close();
		close();
	}
}

void PrintDialog::printClicked(void)
{
	createPrinterObj();
	QPrintDialog printDialog(qprinter, this);
	if (printDialog.exec() == QDialog::Accepted) {
		connect(printer, SIGNAL(progessUpdated(int)), progressBar, SLOT(setValue(int)));
		printer->print();
		close();
	}
}

void PrintDialog::onPaintRequested(QPrinter*)
{
	createPrinterObj();
	connect(printer, SIGNAL(progessUpdated(int)), progressBar, SLOT(setValue(int)));
	printer->print();
	progressBar->setValue(0);
	disconnect(printer, SIGNAL(progessUpdated(int)), progressBar, SLOT(setValue(int)));
}
#endif
