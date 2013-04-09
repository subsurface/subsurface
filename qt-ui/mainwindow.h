#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>


namespace Ui
{
	class MainWindow;
}

class DiveInfo;
class DiveNotes;
class Stats;
class Equipment;

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
	MainWindow();

private Q_SLOTS:

	void on_actionNew_triggered();

private:
	Ui::MainWindow *ui;
};

#endif
