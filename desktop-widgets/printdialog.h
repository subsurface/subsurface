// SPDX-License-Identifier: GPL-2.0
#ifndef PRINTDIALOG_H
#define PRINTDIALOG_H

#ifndef NO_PRINTING
#include <QDialog>
#include "templateedit.h"
#include "printoptions.h"

struct dive;
class Printer;
class QPrinter;
class QProgressBar;
class PrintOptions;
class PrintLayout;

// should be based on a custom QPrintDialog class
class PrintDialog : public QDialog {
	Q_OBJECT

public:
	// If singleDive is non-null, print only that single dive
	explicit PrintDialog(dive *singleDive, const QString &filename, QWidget *parent = 0);
	~PrintDialog();

private:
	dive *singleDive;
	QString filename;
	PrintOptions *optionsWidget;
	QProgressBar *progressBar;
	Printer *printer;
	QPrinter *qprinter;
	struct print_options printOptions;
	struct template_options templateOptions;

private
slots:
	void onFinished();
	void previewClicked();
	void exportHtmlClicked();
	void printClicked();
	void onPaintRequested(QPrinter *);
	void createPrinterObj();
};
#endif
#endif // PRINTDIALOG_H
