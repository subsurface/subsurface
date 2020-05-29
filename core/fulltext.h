// SPDX-License-Identifier: GPL-2.0

// A class for performing full text searches on dives.
// Search is case-insensitive. Even though QString has many design
// issues such as COW semantics and UTF-16 encoding, it provides
// platform independence and reasonable performance. Therefore,
// this is based in QString instead of std::string.
//
// To make this accessible from C, this does manual memory management:
// Every dive is associated with a cache of words. Thus, when deleting
// a dive, a function freeing that data has to be called.

#ifndef FULLTEXT_H
#define FULLTEXT_H

// 1) The C-accessible interface

#ifdef __cplusplus
extern "C" {
#endif

struct full_text_cache;
struct dive;
void fulltext_register(struct dive *d); // Note: can be called repeatedly
void fulltext_unregister(struct dive *d); // Note: can be called repeatedly
void fulltext_unregister_all(); // Unregisters all dives in the dive table
void fulltext_populate(); // Registers all dives in the dive table

#ifdef __cplusplus
}
#endif

// 2) The C++-only interface
#ifdef __cplusplus

#include <QString>
#include <vector>

enum class StringFilterMode {
	SUBSTRING = 0,
	STARTSWITH = 1,
	EXACT = 2
};

// A fulltext query. Basically a list of normalized words we search for
struct FullTextQuery {
	std::vector<QString> words;
	QString originalQuery; // Remember original query, which will be written to the log
	FullTextQuery &operator=(const QString &); // Initialize by assigning a user-provided search string
	bool doit() const; // true if we should to a fulltext search
};

// Describes the result of a fulltext search
struct FullTextResult {
	std::vector<dive *> dives;
	bool dive_matches(const struct dive *d) const;
};

// Two search modes:
//	1) Find all dives matching the query.
//	2) Test if a given dive matches the query.
FullTextResult fulltext_find_dives(const FullTextQuery &q, StringFilterMode);
bool fulltext_dive_matches(const struct dive *d, const FullTextQuery &q, StringFilterMode);

#endif
#endif
