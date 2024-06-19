#ifndef DIVEIMPORTEDMODEL_H
#define DIVEIMPORTEDMODEL_H

#include "core/divelist.h"
#include "core/divelog.h"
#include "core/downloadfromdcthread.h"
#include <QAbstractTableModel>

class DiveImportedModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	enum roleTypes { DateTime = Qt::UserRole + 1, Duration, Depth, Selected};

	DiveImportedModel(QObject *parent = 0);
	int columnCount(const QModelIndex& index = QModelIndex()) const;
	int rowCount(const QModelIndex& index = QModelIndex()) const;
	QVariant data(const QModelIndex& index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	void setImportedDivesIndices(int first, int last);
	Qt::ItemFlags flags(const QModelIndex &index) const;
	Q_INVOKABLE void clearTable();
	QHash<int, QByteArray> roleNames() const;
	void deleteDeselected();
	struct divelog consumeTables(); // Returns downloaded tables and resets model.

	int numDives() const;
	Q_INVOKABLE void recordDives(int flags = import_flags::prefer_imported | import_flags::is_downloaded);
	Q_INVOKABLE void startDownload();
	Q_INVOKABLE void waitForDownload();

	DownloadThread thread;
public
slots:
	void changeSelected(QModelIndex clickedIndex);
	void selectRow(int row);
	void selectAll();
	void selectNone();

private
slots:
	void downloadThreadFinished();

signals:
	void downloadFinished();

private:
	std::vector<char> checkStates; // char instead of bool to avoid silly pessimization of std::vector.
	struct divelog log;
};

#endif
