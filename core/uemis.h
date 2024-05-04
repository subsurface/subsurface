// SPDX-License-Identifier: GPL-2.0
/*
 * defines and prototypes for the uemis Zurich SDA file parser
 */

#ifndef UEMIS_H
#define UEMIS_H

#include "libdivecomputer.h" // for device_data_t, which is a typedef, not a struct :(

#include <string>
#include <string_view>
#include <unordered_map>
#include <cstdint>

struct dive;
struct uemis_sample;
struct dive_site;

struct uemis {
	void parse_divelog_binary(std::string_view base64, struct dive *dive);
	int get_weight_unit(uint32_t diveid) const;
	void mark_divelocation(int diveid, int divespot, struct dive_site *ds);
	void set_divelocation(int divespot, const std::string &text, double longitude, double latitude);
	int get_divespot_id_by_diveid(uint32_t diveid) const;
private:
	struct helper {
		int lbs = 9;
		int divespot = 9;
		struct dive_site *dive_site = nullptr;
	};
	// Use a hash-table (std::unordered_map) to access dive information.
	// Might also use a balanced binary tree (std::map) or a sorted array (std::vector).
	std::unordered_map<uint32_t, helper> helper_table;

	static void event(struct dive *dive, struct divecomputer *dc, struct sample *sample, const uemis_sample *u_sample);
	struct helper &get_helper(uint32_t diveid);
	void weight_unit(int diveid, int lbs);
};

std::string do_uemis_import(device_data_t *data);

#endif // UEMIS_H
