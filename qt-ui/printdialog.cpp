#include "printdialog.h"

#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPrintPreviewDialog>

PrintDialog *PrintDialog::instance()
{
	static PrintDialog *self = new PrintDialog();
	self->setAttribute(Qt::WA_QuitOnClose, false);
	return self;
}

PrintDialog::PrintDialog(QWidget *parent, Qt::WindowFlags f)
{
	// options template (are we storing these in the settings?)
	struct options tempOptions = {options::PRETTY, 0, 2, false, 65, 15, 12};
	printOptions = tempOptions;

	// create a print layout and pass the printer and options
	printLayout = new PrintLayout(this, &printer, &printOptions);

	/* temporary.
	 * add the PrintOptions widget and a Print button for testing purposes. */
	optionsWidget = new PrintOptions(this, &printOptions);

	QVBoxLayout *layout = new QVBoxLayout(this);
	setLayout(layout);
	layout->addWidget(optionsWidget);

	QPushButton *printButton = new QPushButton(tr("&Print"));
	connect(printButton, SIGNAL(clicked(bool)), this, SLOT(printClicked()));
	layout->addWidget(printButton);

	setFixedSize(520, 500);
	setWindowTitle("Print");
}

void PrintDialog::runDialog()
{
	exec();
}

void PrintDialog::printClicked(void)
{
	// printer.setOutputFileName("print.pdf");
	// printer.setOutputFormat(QPrinter::PdfFormat);
	// temporary: use a preview dialog
	printer.setResolution(300);
	QPrintPreviewDialog previewDialog(&printer, this);
    QObject::connect(&previewDialog, SIGNAL(paintRequested(QPrinter *)), this, SLOT(onPaintRequested(QPrinter *)));
    previewDialog.exec();
}

void PrintDialog::onPaintRequested(QPrinter *printerPtr)
{
	printLayout->print();
}
