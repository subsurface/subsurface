// SPDX-License-Identifier: GPL-2.0

#include "fulltext.h"
#include "dive.h"
#include "divelog.h"
#include "divesite.h"
#include "tag.h"
#include "trip.h"
#include "qthelper.h"
#include <QLocale>
#include <map>

// The FullText-search class
class FullText {
	std::map<QString, std::vector<dive *>> words; // Dives that belong to each word
public:
	void populate(); // Rebuild from current dive_table
	void registerDive(struct dive *d); // Note: can be called repeatedly
	void unregisterDive(struct dive *d); // Note: can be called repeatedly
	void unregisterAll(); // Unregister all dives in the dive table
	FullTextResult find(const FullTextQuery &q, StringFilterMode mode) const; // Find dives matchin all words.
private:
	void registerWords(struct dive *d, const std::vector<QString> &w);
	void unregisterWords(struct dive *d, const std::vector<QString> &w);
	std::vector<dive *> findDives(const QString &s, StringFilterMode mode) const; // Find dives matching a given word.
};

// This class doesn't depend on any other objects, we might just initialize it at startup.
static FullText self;

// C-interface functions

void fulltext_register(struct dive *d)
{
	self.registerDive(d);
}

void fulltext_unregister(struct dive *d)
{
	self.unregisterDive(d);
}

void fulltext_unregister_all()
{
	self.unregisterAll();
}

void fulltext_populate()
{
	self.populate();
}

// C++-only interface functions
FullTextResult fulltext_find_dives(const FullTextQuery &q, StringFilterMode mode)
{
	return self.find(q, mode);
}

// Check whether a single dive matches the fulltext criterion
bool fulltext_dive_matches(const struct dive *d, const FullTextQuery &q, StringFilterMode mode)
{
	if (!q.doit())
		return true;
	if (!d->full_text)
		return false;
	auto matchFunc =
		mode == StringFilterMode::EXACT ? [](const QString &s1, const QString &s2) { return s1 == s2; } :
		mode == StringFilterMode::STARTSWITH ? [](const QString &s1, const QString &s2) { return s1.startsWith(s2); } :
		/* mode == StringFilterMode::SUBSTRING ? */ [](const QString &s1, const QString &s2) { return s1.contains(s2); };
	const std::vector<QString> &words = d->full_text->words;
	for (const QString &search: q.words) {
		if (std::any_of(words.begin(), words.end(), [&search,matchFunc](const QString &w) { return matchFunc(w, search); }))
			return true;
	}
	return false;
}

// Class implementation

// Take a text and tokenize it into words. Normalize the words to the base
// upper case base character (e.g. 'ℓ' to 'L') and add to a given list,
// if not already in list.
// We might think about limiting the lower size of words we store.
// Note: we convert to QString before tokenization because we rely in
// Qt's isPunct() function.
static void tokenize(QString s, std::vector<QString> &res)
{
	if (s.isEmpty())
		return;

	QLocale loc;
	int size = s.size();
	int pos = 0;
	while (pos < size) {
		// Skip whitespace and punctuation
		while (s[pos].isSpace() || s[pos].isPunct()) {
			if (++pos >= size)
				return;
		}
		int end = pos;
		while (end < size && !s[end].isSpace() && !s[end].isPunct())
			++end;
		QString word = s.mid(pos, end - pos);
		word = word.normalized(QString::NormalizationForm_KD);
		word = loc.toUpper(word);
		pos = end;

		if (find(res.begin(), res.end(), word) == res.end())
			res.push_back(word);
	}
}

// Get all words of a dive
static std::vector<QString> getWords(const dive *d)
{
	std::vector<QString> res;
	tokenize(QString::fromStdString(d->notes), res);
	tokenize(QString::fromStdString(d->diveguide), res);
	tokenize(QString::fromStdString(d->buddy), res);
	tokenize(QString::fromStdString(d->suit), res);
	for (const divetag *tag: d->tags)
		tokenize(QString::fromStdString(tag->name), res);
	for (auto &cyl: d->cylinders)
		tokenize(QString::fromStdString(cyl.type.description), res);
	for (auto &ws: d->weightsystems)
		tokenize(QString::fromStdString(ws.description), res);
	// TODO: We should tokenize all dive-sites and trips first and then
	// take the tokens from a cache.
	if (d->dive_site) {
		tokenize(QString::fromStdString(d->dive_site->name), res);
		std::string country = taxonomy_get_country(d->dive_site->taxonomy);
		if (!country.empty())
			tokenize(country.c_str(), res);
	}
	// TODO: We should index trips separately!
	if (d->divetrip)
		tokenize(QString::fromStdString(d->divetrip->location), res);
	return res;
}

void FullText::populate()
{
	// we want this to be two calls as the second text is overwritten below by the lines starting with "\r"
	uiNotification(QObject::tr("Create full text index"));
	uiNotification(QObject::tr("start processing"));
	for (auto &d: divelog.dives)
		registerDive(d.get());
	uiNotification(QObject::tr("%1 dives processed").arg(divelog.dives.size()));
}

void FullText::registerDive(struct dive *d)
{
	if (d->full_text)
		unregisterWords(d, d->full_text->words);
	else
		d->full_text = std::make_unique<full_text_cache>();
	d->full_text->words = getWords(d);
	registerWords(d, d->full_text->words);
}

void FullText::unregisterDive(struct dive *d)
{
	if (!d->full_text)
		return;
	unregisterWords(d, d->full_text->words);
	d->full_text.reset();
}

void FullText::unregisterAll()
{
	for (auto &d: divelog.dives)
		d->full_text.reset();
	words.clear();
}

// Register words of a dive.
void FullText::registerWords(struct dive *d, const std::vector<QString> &w)
{
	for (const QString &word: w) {
		std::vector<dive *> &entry = words[word];
		if (std::find(entry.begin(), entry.end(), d) == entry.end())
			entry.push_back(d);
	}
}

// Unregister words of a dive.
void FullText::unregisterWords(struct dive *d, const std::vector<QString> &w)
{
	for (const QString &word: w) {
		auto it = words.find(word);
		if (it == words.end()) {
			qWarning("FullText::unregisterWords: didn't find word '%s' in index!?", qPrintable(word));
			continue;
		}
		std::vector<dive *> &entry = it->second;
		entry.erase(std::remove(entry.begin(), entry.end(), d));
		if (entry.empty())
			words.erase(it);
	}
}

// Add dives from second array to first, if not yet there
void combineDives(std::vector<dive *> &to, const std::vector<dive *> &from)
{
	for (dive *d: from) {
		if (std::find(to.begin(), to.end(), d) == to.end())
			to.push_back(d);
	}
}

std::vector<dive *> FullText::findDives(const QString &s, StringFilterMode mode) const
{
	switch (mode) {
	case StringFilterMode::EXACT:
	default: {
		// Try to access a single word
		auto it = words.find(s);
		if (it == words.end())
			return {};
		return it->second;
	}
	case StringFilterMode::STARTSWITH: {
		// Find all words that start with a substring. We use the fact
		// that these words must form a contiguous block, since the words are
		// ordered lexicographically.
		auto it = words.lower_bound(s);
		if (it == words.end() || !it->first.startsWith(s))
			return {};
		std::vector<dive *> res = it->second;
		++it;
		while (it != words.end() && it->first.startsWith(s)) {
			combineDives(res, it->second);
			++it;
		}
		return res;
	}
	case StringFilterMode::SUBSTRING: {
		// Find all words that contain a substring. Here, we have to check all words!
		std::vector<dive *> res;
		for (auto it = words.begin(); it != words.end(); ++it) {
			if (it->first.contains(s))
				combineDives(res, it->second);
		}
		return res;
	}
	}
}

FullTextResult FullText::find(const FullTextQuery &q, StringFilterMode mode) const
{
	if (q.words.empty())
		return FullTextResult();

	std::vector<dive *> res = findDives(q.words[0], mode);
	for (size_t i = 1; i < q.words.size(); ++i) {
		std::vector<dive *> res2 = findDives(q.words[i], mode);
		// Remove dives from res that are not in res2
		res.erase(std::remove_if(res.begin(), res.end(),
				[&res2] (dive *d) { return std::find(res2.begin(), res2.end(), d) == res2.end(); }), res.end());
	}

	return { std::move(res) };
}

FullTextQuery &FullTextQuery::operator=(const QString &s)
{
	originalQuery = s;
	words.clear();
	tokenize(s, words);
	return *this;
}

bool FullTextQuery::doit() const
{
	return !words.empty();
}

bool FullTextResult::dive_matches(const struct dive *d) const
{
	return std::find(dives.begin(), dives.end(), d) != dives.end();
}
