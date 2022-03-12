// SPDX-License-Identifier: GPL-2.0
#include "globals.h"

#include <vector>

static std::vector<GlobalObjectBase *> global_objects;

void register_global_internal(GlobalObjectBase *o)
{
	global_objects.push_back(o);
}

void free_globals()
{
	// We free the objects by hand, so that we can free them in reverse
	// order of creation. AFAIK, order-of-destruction is implementantion-defined
	// for std::vector<>.
	for (auto it = global_objects.rbegin(); it != global_objects.rend(); ++it)
		delete *it;
	global_objects.clear();
}
