#include "printdialog.h"

#include <QDebug>
#include <QPushButton>
#include <QVBoxLayout>

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
	// nop for now
}
