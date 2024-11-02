// SPDX-License-Identifier: GPL-2.0
#include "xmlparams.h"

struct xml_params *alloc_xml_params()
{
	return new xml_params;
}

void free_xml_params(struct xml_params *params)
{
	delete params;
}

void xml_params_resize(struct xml_params *params, int count)
{
	params->items.resize(count);
}

void xml_params_add(struct xml_params *params, const char *key, const char *value)
{
	xml_params_add(params, std::string(key), std::string(value));
}

void xml_params_add(struct xml_params *params, const std::string &key, const std::string &value)
{
	params->items.push_back({key, value});
}

void xml_params_add_int(struct xml_params *params, const char *key, int value)
{
	params->items.push_back({std::string(key), std::to_string(value)});
}

int xml_params_count(const struct xml_params *params)
{
	return (int)params->items.size();
}

const char *xml_params_get_key(const struct xml_params *params, int idx)
{
	return params->items[idx].first.c_str();
}

const char *xml_params_get_value(const struct xml_params *params, int idx)
{
	return params->items[idx].second.c_str();
}

extern void xml_params_set_value(struct xml_params *params, int idx, const char *value)
{
	if (idx < 0 || idx >= (int)params->items.size())
		return;
	params->items[idx].second = value;
}

const char **xml_params_get(const struct xml_params *params)
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
