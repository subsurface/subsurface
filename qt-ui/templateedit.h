#ifndef TEMPLATEEDIT_H
#define TEMPLATEEDIT_H

#include <QDialog>

namespace Ui {
class TemplateEdit;
}

class TemplateEdit : public QDialog
{
	Q_OBJECT

public:
	explicit TemplateEdit(QWidget *parent = 0);
	~TemplateEdit();

private:
	Ui::TemplateEdit *ui;
};

#endif // TEMPLATEEDIT_H
