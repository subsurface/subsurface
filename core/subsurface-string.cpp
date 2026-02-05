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

// Poor man's trim function for std::string_view
void trim(std::string_view &sv)
{
	while (!sv.empty() && isspace(sv.front()))
		sv.remove_prefix(1);
	while (!sv.empty() && isspace(sv.back()))
		sv.remove_suffix(1);
}

std::string trimmed(std::string_view sv)
{
	trim(sv);
	return std::string(sv);
}
