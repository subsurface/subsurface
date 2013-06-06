#ifndef SUBSURFACEWEBSERVICES_H
#define SUBSURFACEWEBSERVICES_H

#include <QDialog>
#include <QNetworkReply>
#include <libxml/tree.h>

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
	void setStatusText(int status);
	void download_dialog_traverse_xml(xmlNodePtr node, unsigned int *download_status);
	unsigned int download_dialog_parse_response(const QByteArray& length);

	explicit SubsurfaceWebServices(QWidget* parent = 0, Qt::WindowFlags f = 0);
	Ui::SubsurfaceWebServices *ui;
	QNetworkReply *reply;
	QNetworkAccessManager *manager;
	QByteArray downloadedData;
};

#endif