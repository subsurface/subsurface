#ifndef TEMPLATEEDIT_H
#define TEMPLATEEDIT_H

#include <QDialog>

struct template_options {
	int font_index;
	int color_palette_index;
	double font_size;
	double line_spacing;
};

namespace Ui {
class TemplateEdit;
}

class TemplateEdit : public QDialog
{
	Q_OBJECT

public:
	explicit TemplateEdit(QWidget *parent, struct template_options *templateOptions);
	~TemplateEdit();
private slots:
	void on_fontsize_valueChanged(int font_size);

	void on_linespacing_valueChanged(double line_spacing);

	void on_fontSelection_currentIndexChanged(int index);

	void on_colorpalette_currentIndexChanged(int index);

private:
	Ui::TemplateEdit *ui;
	struct template_options *templateOptions;
};

#endif // TEMPLATEEDIT_H
