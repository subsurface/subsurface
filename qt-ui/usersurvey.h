#ifndef USERSURVEY_H
#define USERSURVEY_H

#include <QDialog>

namespace Ui {
	class UserSurvey;
}

class UserSurvey : public QDialog {
	Q_OBJECT

public:
	explicit UserSurvey(QWidget *parent = 0);
	~UserSurvey();

private
slots:
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();

private:
	Ui::UserSurvey *ui;
};
#endif // USERSURVEY_H
