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
	static QString getVersion();
	static QString getUserAgent();

private
slots:
	void on_buttonBox_accepted();
	void on_buttonBox_rejected();
	void requestReceived();

private:
	Ui::UserSurvey *ui;
	QString os;
};
#endif // USERSURVEY_H
