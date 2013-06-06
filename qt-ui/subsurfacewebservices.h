#ifndef SUBSURFACEWEBSERVICES_H
#define SUBSURFACEWEBSERVICES_H

#include <QDialog>
#include <QNetworkReply>

namespace Ui{
	class SubsurfaceWebServices;
};
class QAbstractButton;
class QNetworkReply;

class SubsurfaceWebServices : public QDialog {
	Q_OBJECT
public:
	static SubsurfaceWebServices* instance();
	void runDialog();
	
private slots:
	void startDownload();
	void buttonClicked(QAbstractButton* button);
	void downloadFinished();
	void downloadError(QNetworkReply::NetworkError error);
	
private:
    explicit SubsurfaceWebServices(QWidget* parent = 0, Qt::WindowFlags f = 0);
    Ui::SubsurfaceWebServices *ui;
	QNetworkReply *reply;
};

#endif