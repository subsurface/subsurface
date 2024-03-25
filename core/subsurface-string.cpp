// SPDX-License-Identifier: GPL-2.0
#include "subsurface-string.h"

std::string join(const std::vector<std::string> &l, const std::string &separator, bool skip_empty)
{
	std::string res;
	bool first = true;
	for (const std::string &s: l) {
		if (skip_empty && l.empty())
			continue;
		if (!first)
			res += separator;
		res += s;
		first = false;
	}
	return res;
}
