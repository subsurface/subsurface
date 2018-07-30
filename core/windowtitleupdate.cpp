// SPDX-License-Identifier: GPL-2.0
#include "windowtitleupdate.h"

WindowTitleUpdate windowTitleUpdate;

extern "C" void updateWindowTitle()
{
	emit windowTitleUpdate.updateTitle();
}
