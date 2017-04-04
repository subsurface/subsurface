#ifndef TAB_BASE_H
#define TAB_BASE_H

#include <QWidget>

struct dive;

class TabBase : public QWidget {
	Q_OBJECT

public:
	TabBase(QWidget *parent);
	virtual void updateData() = 0;
	virtual void clear() = 0;
};

#endif
