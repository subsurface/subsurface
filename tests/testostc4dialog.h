// SPDX-License-Identifier: GPL-2.0
#ifndef TESTOSTC4DIALOG_H
#define TESTOSTC4DIALOG_H
#include <QtTest>
#include <QUrl>

class TestOstc4Dialog : public QObject {
	Q_OBJECT
private slots:
	void hiddenTabsAndReread();
	void continuousSaveSession();
	void transactionThroughSession();
	void liveSessionDestructor();
	void writeWorkerOwnsTransport();
	void readThenWriteWorkersOwnTransports();
	void readWorkerCleansPartialOpen();
	void retrieveUsesOneWorkerConnection();
	void rfcommDisconnectWait();
	void clampedGradientFactorsStayUnchanged();
	void settingsBlockedDuringFirmwareAndReset();
	void openFailureWithoutCloseHook();
	void dialogGuardsAndManualBackupError();
	void closeFailureBlocksNextOperation();
	void closeFailureBlocksOstc3Save();
	void precloseFailureBlocksRead();
	void recordOpenedUrl(QUrl url);
private:
	QUrl openedUrl;
};
#endif
