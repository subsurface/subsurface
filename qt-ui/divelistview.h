#ifndef DIVELISTVIEW_H
#define DIVELISTVIEW_H

/*! A view subclass for use with dives

  Note: calling this a list view might be misleading?


*/

#include <QTreeView>

class DiveListView : public QTreeView
{
public:
	DiveListView(QWidget *parent = 0);
};

#endif // DIVELISTVIEW_H
