#ifndef PREFERENCES_GRAPH_H
#define PREFERENCES_GRAPH_H

#include "abstractpreferenceswidget.h"

namespace Ui {
	class PreferencesGraph;
}

class PreferencesGraph : public AbstractPreferencesWidget {
	Q_OBJECT
public:
	PreferencesGraph();
	virtual ~PreferencesGraph();
	virtual void refreshSettings();
	virtual void syncSettings();

private slots:
	void on_gflow_valueChanged(int gf);
	void on_gfhigh_valueChanged(int gf);

private:
	Ui::PreferencesGraph *ui;

};

#endif