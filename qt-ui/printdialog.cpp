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

PrintDialog::PrintDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	// options template (are we storing these in the settings?)
	struct options tempOptions = {options::PRETTY, 0, 2, false, 65, 15, 12};
	printOptions = tempOptions;

	// create a print layout and pass the printer and options
	printLayout = new PrintLayout(this, &printer, &printOptions);

	// create a print options object and pass our options struct
	optionsWidget = new PrintOptions(this, &printOptions);

	QVBoxLayout *layout = new QVBoxLayout(this);
	setLayout(layout);

	QHBoxLayout *hLayout = new QHBoxLayout();
	layout->addLayout(hLayout);

	QPushButton *previewButton = new QPushButton(tr("&Preview"));
	connect(previewButton, SIGNAL(clicked(bool)), this, SLOT(previewClicked()));
	hLayout->addWidget(previewButton);

	QPushButton *printButton = new QPushButton(tr("P&rint"));
	connect(printButton, SIGNAL(clicked(bool)), this, SLOT(printClicked()));
	hLayout->addWidget(printButton);

	QPushButton *closeButton = new QPushButton(tr("&Close"));
	connect(closeButton, SIGNAL(clicked(bool)), this, SLOT(accept()));
	hLayout->addWidget(closeButton);

	progressBar = new QProgressBar();
	connect(printLayout, SIGNAL(signalProgress(int)), progressBar, SLOT(setValue(int)));
	progressBar->setMinimum(0);
	progressBar->setMaximum(100);
	progressBar->setValue(0);
	progressBar->setTextVisible(false);
	layout->addWidget(progressBar);

	layout->addWidget(optionsWidget);

	setFixedSize(520, 350);
	setWindowTitle(tr("Print"));
	setWindowIcon(QIcon(":subsurface-icon"));
}

void PrintDialog::previewClicked(void)
{
	QPrintPreviewDialog previewDialog(&printer, this);
	connect(&previewDialog, SIGNAL(paintRequested(QPrinter *)), this, SLOT(onPaintRequested(QPrinter *)));
	previewDialog.exec();
}

void PrintDialog::printClicked(void)
{
	QPrintDialog printDialog(&printer, this);
	if (printDialog.exec() == QDialog::Accepted)
		printLayout->print();
}

void PrintDialog::onPaintRequested(QPrinter *printerPtr)
{
	printLayout->print();
}
