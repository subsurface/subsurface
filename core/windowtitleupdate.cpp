// SPDX-License-Identifier: GPL-2.0
#include "windowtitleupdate.h"

WindowTitleUpdate windowTitleUpdate;

void updateWindowTitle()
{
	emit windowTitleUpdate.updateTitle();
}
