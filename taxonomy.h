#ifndef TAXONOMY_H
#define TAXONOMY_H

#ifdef __cplusplus
extern "C" {
#endif

enum taxonomy_category {
	TC_NONE,
	TC_OCEAN,
	TC_COUNTRY,
	TC_ADMIN_L1,
	TC_ADMIN_L2,
	TC_LOCALNAME,
	TC_ADMIN_L3,
	TC_NR_CATEGORIES
};

extern char *taxonomy_category_names[TC_NR_CATEGORIES];
extern char *taxonomy_api_names[TC_NR_CATEGORIES];

struct taxonomy {
	int category;		/* the category for this tag: ocean, country, admin_l1, admin_l2, localname, etc */
	const char *value;	/* the value returned, parsed, or manually entered for that category */
	enum { GEOCODED, PARSED, MANUAL, COPIED } origin;
};

/* the data block contains 3 taxonomy structures - unused ones have a tag value of NONE */
struct taxonomy_data {
	int nr;
	struct taxonomy *category;
};

struct taxonomy *alloc_taxonomy();
void free_taxonomy(struct taxonomy_data *t);
int taxonomy_index_for_category(struct taxonomy_data *t, enum taxonomy_category cat);

#ifdef __cplusplus
}
#endif
#endif // TAXONOMY_H
