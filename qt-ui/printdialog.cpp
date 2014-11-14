#include "printdialog.h"
#include "printoptions.h"
#include "printlayout.h"
#include "mainwindow.h"

#include <QDebug>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPrintPreviewDialog>
#include <QPrintDialog>
#include <QShortcut>
#include <QPrinterInfo>
#include <QMessageBox>
#include <QSettings>

#if QT_VERSION >= 0x050300
#include <QMarginsF>
#endif

#define SETTINGS_GROUP "PrintDialog"

PrintDialog::PrintDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	// check if the options were previously stored in the settings; if not use some defaults.
	QSettings s;
	bool stored = s.childGroups().contains(SETTINGS_GROUP);
	if (!stored) {
		printOptions.type = print_options::PRETTY;
		printOptions.print_selected = true;
		printOptions.color_selected = true;
		printOptions.notes_up = false;
		printOptions.landscape = false;
		memset(printOptions.margins, 0, sizeof(printOptions.margins));
	} else {
		s.beginGroup(SETTINGS_GROUP);
		printOptions.type = (print_options::print_type)s.value("type").toInt();
		printOptions.print_selected = s.value("print_selected").toBool();
		printOptions.color_selected = s.value("color_selected").toBool();
		printOptions.notes_up = s.value("notes_up").toBool();
		printOptions.landscape = s.value("landscape").toBool();
		printOptions.margins[0] = s.value("margin_left").toInt();
		printOptions.margins[1] = s.value("margin_top").toInt();
		printOptions.margins[2] = s.value("margin_right").toInt();
		printOptions.margins[3] = s.value("margin_bottom").toInt();
		printer.setOrientation((QPrinter::Orientation)printOptions.landscape);
#if QT_VERSION >= 0x050300
		QMarginsF margins;
		margins.setLeft(printOptions.margins[0]);
		margins.setTop(printOptions.margins[1]);
		margins.setRight(printOptions.margins[2]);
		margins.setBottom(printOptions.margins[3]);
		printer.setPageMargins(margins, QPageLayout::Millimeter);
#else
		printer.setPageMargins(
			printOptions.margins[0],
			printOptions.margins[1],
			printOptions.margins[2],
			printOptions.margins[3],
			QPrinter::Millimeter
		);
#endif
	}

	// create a print layout and pass the printer and options
	printLayout = new PrintLayout(this, &printer, &printOptions);

	// create a print options object and pass our options struct
	optionsWidget = new PrintOptions(this, &printOptions);

	QVBoxLayout *layout = new QVBoxLayout(this);
	setLayout(layout);

	layout->addWidget(optionsWidget);

	progressBar = new QProgressBar();
	connect(printLayout, SIGNAL(signalProgress(int)), progressBar, SLOT(setValue(int)));
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

	QDialogButtonBox *buttonBox = new QDialogButtonBox;
	buttonBox->addButton(QDialogButtonBox::Cancel);
	buttonBox->addButton(printButton, QDialogButtonBox::AcceptRole);
	buttonBox->addButton(previewButton, QDialogButtonBox::ActionRole);

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

void PrintDialog::onFinished()
{
	// save the settings
	QSettings s;
	s.beginGroup(SETTINGS_GROUP);
	s.setValue("type", printOptions.type);
	s.setValue("print_selected", printOptions.print_selected);
	s.setValue("color_selected", printOptions.color_selected);
	s.setValue("notes_up", printOptions.notes_up);
#if QT_VERSION >= 0x050300
	s.setValue("landscape", (bool)printer.pageLayout().orientation());
	QMarginsF margins = printer.pageLayout().margins(QPageLayout::Millimeter);
	s.setValue("margin_left", margins.left());
	s.setValue("margin_top", margins.top());
	s.setValue("margin_right", margins.right());
	s.setValue("margin_bottom", margins.bottom());
#else
	s.setValue("landscape", (bool)printer.orientation());
	qreal left, top, right, bottom;
	printer.getPageMargins(&left, &top, &right, &bottom, QPrinter::Millimeter);
	s.setValue("margin_left", (int)left);
	s.setValue("margin_top", (int)top);
	s.setValue("margin_right", (int)right);
	s.setValue("margin_bottom", (int)bottom);
#endif
}

void PrintDialog::previewClicked(void)
{
	QPrintPreviewDialog previewDialog(&printer, this, Qt::Window
		| Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint
		| Qt::WindowTitleHint);

	connect(&previewDialog, SIGNAL(paintRequested(QPrinter *)), this, SLOT(onPaintRequested(QPrinter *)));
	previewDialog.exec();
}

void PrintDialog::printClicked(void)
{
	QPrintDialog printDialog(&printer, this);
	if (printDialog.exec() == QDialog::Accepted){
		printLayout->print();
		close();
	}
}

void PrintDialog::onPaintRequested(QPrinter *printerPtr)
{
	printLayout->print();
}
