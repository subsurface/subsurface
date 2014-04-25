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

PrintDialog::PrintDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	// options template (are we storing these in the settings?)
	struct options tempOptions = { options::PRETTY, 0, 2, false, 65, 15, 12 };
	printOptions = tempOptions;

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

	setFixedSize(550, 400);
	setWindowTitle(tr("Print"));
	setWindowIcon(QIcon(":subsurface-icon"));

	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
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
