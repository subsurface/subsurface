// SPDX-License-Identifier: GPL-2.0
#include "xmlparams.h"

extern "C" struct xml_params *alloc_xml_params()
{
	return new xml_params;
}

extern "C" void free_xml_params(struct xml_params *params)
{
	delete params;
}

extern "C" void xml_params_resize(struct xml_params *params, int count)
{
	params->items.resize(count);
}

extern "C" void xml_params_add(struct xml_params *params, const char *key, const char *value)
{
	params->items.push_back({ std::string(key), std::string(value) });
}

extern "C" void xml_params_add_int(struct xml_params *params, const char *key, int value)
{
	params->items.push_back({ std::string(key), std::to_string(value) });
}

extern "C" int xml_params_count(const struct xml_params *params)
{
	return (int)params->items.size();
}

extern "C" const char *xml_params_get_key(const struct xml_params *params, int idx)
{
	return params->items[idx].first.c_str();
}

extern "C" const char *xml_params_get_value(const struct xml_params *params, int idx)
{
	return params->items[idx].second.c_str();
}

extern void xml_params_set_value(struct xml_params *params, int idx, const char *value)
{
	if (idx < 0 || idx >= (int)params->items.size())
		return;
	params->items[idx].second = value;
}

extern "C" const char **xml_params_get(const struct xml_params *params)
{
	if (!params)
		return nullptr;
	params->data.resize(params->items.size() * 2 + 1);
	for (size_t i = 0; i < params->items.size(); ++i) {
		params->data[i * 2] = params->items[i].first.c_str();
		params->data[i * 2 + 1] = params->items[i].second.c_str();
	}
	params->data[params->items.size() * 2] = nullptr;
	return params->data.data();
}
