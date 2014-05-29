#ifndef CONFIGUREDIVECOMPUTERDIALOG_H
#define CONFIGUREDIVECOMPUTERDIALOG_H

#include <QDialog>
#include <QStringListModel>
#include "../libdivecomputer.h"
namespace Ui {
class ConfigureDiveComputerDialog;
}

class ConfigureDiveComputerDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ConfigureDiveComputerDialog(QWidget *parent = 0);
	~ConfigureDiveComputerDialog();

private slots:
	void on_vendor_currentIndexChanged(const QString &vendor);

	void on_product_currentIndexChanged(const QString &product);

private:
	Ui::ConfigureDiveComputerDialog *ui;

	QStringList vendorList;
	QHash<QString, QStringList> productList;
	QHash<QString, dc_descriptor_t *> descriptorLookup;

	QStringListModel *vendorModel;
	QStringListModel *productModel;
	void fill_computer_list();
	void fill_device_list(int dc_type);
};

#endif // CONFIGUREDIVECOMPUTERDIALOG_H
