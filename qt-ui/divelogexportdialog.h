#ifndef DIVELOGEXPORTDIALOG_H
#define DIVELOGEXPORTDIALOG_H

#include <QDialog>

namespace Ui {
class DiveLogExportDialog;
}

class DiveLogExportDialog : public QDialog
{
	Q_OBJECT

public:
	explicit DiveLogExportDialog(QWidget *parent = 0);
	~DiveLogExportDialog();

private slots:
	void on_buttonBox_accepted();

private:
	Ui::DiveLogExportDialog *ui;
};

#endif // DIVELOGEXPORTDIALOG_H
