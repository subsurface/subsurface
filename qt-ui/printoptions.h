#ifndef PRINTOPTIONS_H
#define PRINTOPTIONS_H

#include <QWidget>
#include "../display.h"

namespace Ui {
	class PrintOptions;
};

// should be based on a custom QPrintDialog class
class PrintOptions : public QWidget {
Q_OBJECT

public:
	static PrintOptions *instance();

private:
	explicit PrintOptions(QWidget *parent = 0, Qt::WindowFlags f = 0);
	Ui::PrintOptions *ui;
};

#endif
