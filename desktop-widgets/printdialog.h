#ifndef PRINTDIALOG_H
#define PRINTDIALOG_H

#ifndef NO_PRINTING
#include <QDialog>
#include <QPrinter>
#include "printoptions.h"
#include "printer.h"
#include "templateedit.h"

class QProgressBar;
class PrintOptions;
class PrintLayout;

// should be based on a custom QPrintDialog class
class PrintDialog : public QDialog {
	Q_OBJECT

public:
	explicit PrintDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);

private:
	PrintOptions *optionsWidget;
	QProgressBar *progressBar;
	Printer *printer;
	QPrinter qprinter;
	struct print_options printOptions;
	struct template_options templateOptions;

private
slots:
	void onFinished();
	void previewClicked();
	void printClicked();
	void onPaintRequested(QPrinter *);
};
#endif
#endif // PRINTDIALOG_H
