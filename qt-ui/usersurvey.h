#ifndef USERSURVEY_H
#define USERSURVEY_H

#include <QDialog>
class QNetworkAccessManager;
class QNetworkReply;

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
	void requestReceived(QNetworkReply *reply);

private:
	Ui::UserSurvey *ui;
	QString os;
	QString checkboxes;
	QString suggestions;
	QNetworkAccessManager *manager;
};
#endif // USERSURVEY_H
