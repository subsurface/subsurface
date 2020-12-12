// SPDX-License-Identifier: GPL-2.0
#ifndef PRINTOPTIONS_H
#define PRINTOPTIONS_H

#include <QWidget>

#include "ui_printoptions.h"

struct print_options {
	enum print_type {
		DIVELIST,
		STATISTICS
	} type;
	QString p_template;
	bool print_selected;
	bool color_selected;
	bool landscape;
	int resolution;
};

struct template_options {
	int font_index;
	int color_palette_index;
	int border_width;
	double font_size;
	double line_spacing;
	struct color_palette_struct {
		QColor color1;
		QColor color2;
		QColor color3;
		QColor color4;
		QColor color5;
		QColor color6;
		bool operator!=(const color_palette_struct &other) const {
			return other.color1 != color1
					|| other.color2 != color2
					|| other.color3 != color3
					|| other.color4 != color4
					|| other.color5 != color5
					|| other.color6 != color6;
		}
	} color_palette;
	bool operator!=(const template_options &other) const {
		return other.font_index != font_index
				|| other.color_palette_index != color_palette_index
				|| other.font_size != font_size
				|| other.line_spacing != line_spacing
				|| other.border_width != border_width
				|| other.color_palette != color_palette;
	}
 };

extern template_options::color_palette_struct ssrf_colors, almond_colors, blueshades_colors, custom_colors;

enum color_palette {
	SSRF_COLORS,
	ALMOND,
	BLUESHADES,
	CUSTOM
};

// should be based on a custom QPrintDialog class
class PrintOptions : public QWidget {
	Q_OBJECT

public:
	explicit PrintOptions(QWidget *parent, print_options &printOpt, template_options &templateOpt);
	void setup();
	QString getSelectedTemplate();

private:
	Ui::PrintOptions ui;
	print_options &printOptions;
	template_options &templateOptions;
	bool hasSetupSlots;
	QString lastImportExportTemplate;
	void setupTemplates();

private
slots:
	void printInColorClicked(bool check);
	void printSelectedClicked(bool check);
	void on_radioStatisticsPrint_toggled(bool check);
	void on_radioDiveListPrint_toggled(bool check);
	void on_printTemplate_currentIndexChanged(int index);
	void on_editButton_clicked();
	void on_importButton_clicked();
	void on_exportButton_clicked();
	void on_deleteButton_clicked();
};

#endif // PRINTOPTIONS_H
