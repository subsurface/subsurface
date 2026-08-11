// SPDX-License-Identifier: GPL-2.0
#ifndef SUUNTOGASASSIGNDIALOG_H
#define SUUNTOGASASSIGNDIALOG_H

#include "core/file.h"

#include <QDialog>
#include <vector>

class QTableWidget;

// AI-generated (Claude)
/* Shown after importing Suunto JSON dive log(s) that switched gases mid-dive
 * but carried no gas-mix data (no Header.Diving.Gases entry, and no paired
 * FIT file to patch it in). Lets the user fill in the O2/He fraction for
 * each cylinder that a GasSwitch event created but that never resolved to an
 * actual mix, before the dives are committed to the log. */
class SuuntoGasAssignDialog : public QDialog {
	Q_OBJECT
public:
	explicit SuuntoGasAssignDialog(std::vector<suunto_unresolved_gas> unresolved, QWidget *parent = nullptr);

private slots:
	void applyAndAccept();

private:
	std::vector<suunto_unresolved_gas> rows;
	QTableWidget *table;
};

#endif // SUUNTOGASASSIGNDIALOG_H
