/*
 * addweightsystemdialog.h
 *
 * header file for the add weightsystem dialog of Subsurface
 *
 */
#ifndef ADDWEIGHTSYSTEMDIALOG_H
#define ADDWEIGHTSYSTEMDIALOG_H

#include <QDialog>
#include "../dive.h"

namespace Ui{
	class AddWeightsystemDialog;
}

class AddWeightsystemDialog : public QDialog{
	Q_OBJECT
public:
	explicit AddWeightsystemDialog(QWidget* parent = 0);
	void setWeightsystem(weightsystem_t *ws);
	void updateWeightsystem();

private:
	Ui::AddWeightsystemDialog *ui;
	weightsystem_t *currentWeightsystem;
};


#endif
