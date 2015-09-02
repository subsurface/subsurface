#ifndef WINDOWTITLEUPDATE_H
#define WINDOWTITLEUPDATE_H

#include <QObject>

class WindowTitleUpdate : public QObject
{
	Q_OBJECT
public:
	explicit WindowTitleUpdate(QObject *parent = 0);
	~WindowTitleUpdate();
	static WindowTitleUpdate *instance();
	void emitSignal();
signals:
	void updateTitle();
private:
	static WindowTitleUpdate *m_instance;
};

#endif // WINDOWTITLEUPDATE_H
