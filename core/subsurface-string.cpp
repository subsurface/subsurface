// SPDX-License-Identifier: GPL-2.0
#include <string>
#include <algorithm>
#include <cctype>

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

void trim(std::string &s)
{
	const auto wsfront = std::find_if_not(s.begin(), s.end(), [](unsigned char c){ return std::isspace(c); });
	s.erase(s.begin(), wsfront);

	const auto wsback = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char c){ return std::isspace(c); }).base();
	s.erase(wsback, s.end());
}

std::string trimmed(std::string s)
{
	trim(s);
	return s;
}
