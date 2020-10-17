// SPDX-License-Identifier: GPL-2.0
// Small helper class that keeps track of key/value pairs to
// pass to the XML-routines as parameters. Uses C++ for memory
// management, but provides a C interface via anonymous struct.

#ifdef __cplusplus
#include <string>
#include <vector>

struct xml_params {
	std::vector<std::pair<std::string, std::string>> items;
	mutable std::vector<const char *> data;
};

#else

struct xml_params;

#endif

#ifdef __cplusplus
extern "C" {
#endif

// Return values marked as "not stable" may be invalidated when calling
// an xml_params_*() function that takes a non-const xml_params parameter.
extern struct xml_params *alloc_xml_params();
extern void free_xml_params(struct xml_params *params);
extern void xml_params_resize(struct xml_params *params, int count);
extern void xml_params_add(struct xml_params *params, const char *key, const char *value);
extern void xml_params_add_int(struct xml_params *params, const char *key, int value);
extern int xml_params_count(const struct xml_params *params);
extern const char *xml_params_get_key(const struct xml_params *params, int idx); // not stable
extern const char *xml_params_get_value(const struct xml_params *params, int idx); // not stable
extern void xml_params_set_value(struct xml_params *params, int idx, const char *value);
extern const char **xml_params_get(const struct xml_params *params); // not stable

#ifdef __cplusplus
}
#endif
