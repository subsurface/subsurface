// SPDX-License-Identifier: GPL-2.0
// Displays the dive profile. Used by the interactive profile widget
// and the printing/exporting code.
#ifndef PROFILESCENE_H
#define PROFILESCENE_H

#include <QGraphicsScene>

class ProfileScene : public QGraphicsScene {
public:
	ProfileScene();
	~ProfileScene();
};

#endif
