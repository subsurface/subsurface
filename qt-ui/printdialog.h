#ifndef PRINTDIALOG_H
#define PRINTDIALOG_H

#include <QDialog>
#include <QPrinter>
#include "../display.h"

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
	PrintLayout *printLayout;
	QProgressBar *progressBar;
	QPrinter printer;
	struct print_options printOptions;

private
slots:
	void onFinished();
	void previewClicked();
	void printClicked();
	void onPaintRequested(QPrinter *);
};
#endif // PRINTDIALOG_H
