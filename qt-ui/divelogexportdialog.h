#ifndef DIVELOGEXPORTDIALOG_H
#define DIVELOGEXPORTDIALOG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
	class DiveLogExportDialog;
}

class DiveLogExportDialog : public QDialog {
	Q_OBJECT

public:
	explicit DiveLogExportDialog(QWidget *parent = 0);
	~DiveLogExportDialog();

private
slots:
	void on_buttonBox_accepted();
	void on_exportGroup_buttonClicked(QAbstractButton *);

private:
	Ui::DiveLogExportDialog *ui;
	void showExplanation();
};

#endif // DIVELOGEXPORTDIALOG_H
