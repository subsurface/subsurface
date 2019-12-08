// SPDX-License-Identifier: GPL-2.0
#include "plannerShared.h"


plannerShared *plannerShared::instance()
{
    static plannerShared *self = new plannerShared;
    return self;
}
