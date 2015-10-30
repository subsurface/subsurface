#ifndef TEMPLATEEDIT_H
#define TEMPLATEEDIT_H

#include <QDialog>
#include "templatelayout.h"

namespace Ui {
class TemplateEdit;
}

class TemplateEdit : public QDialog
{
	Q_OBJECT

public:
	explicit TemplateEdit(QWidget *parent, struct print_options *printOptions, struct template_options *templateOptions);
	~TemplateEdit();
private slots:
	void on_fontsize_valueChanged(int font_size);

	void on_linespacing_valueChanged(double line_spacing);

	void on_fontSelection_currentIndexChanged(int index);

	void on_colorpalette_currentIndexChanged(int index);

	void on_buttonBox_clicked(QAbstractButton *button);

	void colorSelect(QAbstractButton *button);

private:
	Ui::TemplateEdit *ui;
	QButtonGroup *btnGroup;
	bool editingCustomColors;
	struct template_options *templateOptions;
	struct template_options newTemplateOptions;
	struct print_options *printOptions;
	QString grantlee_template;
	void saveSettings();
	void updatePreview();

};

#endif // TEMPLATEEDIT_H
