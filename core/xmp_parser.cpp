#include "xmp_parser.h"
#include "subsurface-string.h"
#include "subsurface-time.h"

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <cctype>

static timestamp_t parse_xmp_date(const char *date)
{
	// Format: "yyyy-mm-dd[Thh:mm[:ss[.ms]][-05:00]]"
	int year, month, day;
	if (sscanf(date, "%d-%d-%d", &year, &month, &day) != 3)
		return 0;

	int hours = 0, minutes = 0, seconds = 0, milliseconds = 0;
	int timezone = 0;

	// Check for time part
	if ((date = strchr(date, 'T')) != nullptr) {
		++date; // Skip 'T'
		if (sscanf(date, "%d:%d:%d.%d", &hours, &minutes, &seconds, &milliseconds) < 2)
			return 0;

		// Check for timezone part. Note that we simply ignore 'Z' as that
		// means no time zone
		while (*date && *date != '+' && *date != '-')
			++date;
		if (*date) {
			int sign = *date == '+' ? 1 : -1;
			int timezone_hours, timezone_minutes;
			++date;
			if (sscanf(date, "%d:%d", &timezone_hours, &timezone_minutes) != 2)
				return 0;
			timezone = sign * (timezone_hours * 60 + timezone_minutes) * 60;
		}
	}

	// Round to seconds, since our timestamps are in seconds
	if (milliseconds >= 500)
		seconds += 1;

	struct tm tm = { 0 };
	tm.tm_year = year - 1900;
	tm.tm_mon = month - 1;
	tm.tm_mday = day;
	tm.tm_hour = hours;
	tm.tm_min = minutes;
	tm.tm_sec = seconds;

	timestamp_t res = utc_mktime(&tm);
	res += timezone;

	return res;
}

static timestamp_t extract_timestamp_from_attributes(const xmlNode *node)
{
	for (const xmlAttr *p = node->properties; p; p = p->next) {
		const xmlChar *ns = p->ns ? p->ns->prefix : nullptr;

		// Check for xmp::CreateDate property
		if (same_string((const char *)ns, "xmp") && same_string((const char *)p->name, "CreateDate")) {
			// We only support a single property value
			if (!p->children || !p->children->content)
				return 0;
			const char *date = (const char *)p->children->content;
			return parse_xmp_date(date);
		}
	}
	return 0;
}

static timestamp_t extract_timestamp(const xmlNode *firstnode)
{
	// We use a private stack, so that we can return in one go without
	// having to unwind the call-stack. We only recurse to a fixed depth,
	// since the data we are interested in are at a shallow depth.
	// This can be increased on demand.
	static const int max_recursion_depth = 16;
	const xmlNode *stack[max_recursion_depth];
	stack[0] = firstnode;
	int stack_depth = 1;

	while (stack_depth > 0) {
		const xmlNode *node = stack[stack_depth - 1];
		// Parse attributes
		timestamp_t timestamp = extract_timestamp_from_attributes(node);
		if (timestamp)
			return timestamp;

		// Parse content, if not blank node. Content can only be at the second level,
		// since it is always contained in a tag.
		// TODO: We have to cast node to pointer to non-const, since we're supporting
		// old libxml2 versions, where xmlIsBlankNode takes such a pointer. Remove
		// in due course.
		if (!xmlIsBlankNode((xmlNode *)node) && stack_depth >= 2) {
			const xmlNode *parent = stack[stack_depth - 2];
			// If this is a text node and the parent node is exif:DateTimeOriginal, try to parse as date
			if (!node->ns && parent->ns &&
			    same_string((const char *)parent->ns->prefix, "exif") &&
			    same_string((const char *)parent->name, "DateTimeOriginal")) {
				const char *date = (const char *)node->content;
				timestamp_t res = parse_xmp_date(date);
				if(res)
					return res;
			}
		}

		// If there are sub-items and we haven't reached recursion depth, recurse
		if (node->children && stack_depth < max_recursion_depth) {
			stack[stack_depth++] = node->children;
			continue;
		}

		// Advance stack to next node in this level
		while (stack_depth > 0) {
			if ((stack[stack_depth - 1] = stack[stack_depth - 1]->next) != nullptr)
				break;
			// No more nodes at this level -> go up a level.
			--stack_depth;
		}
	}
	return 0;
}

timestamp_t parse_xmp(const char *data, size_t size)
{
	const char *encoding = xmlGetCharEncodingName(XML_CHAR_ENCODING_UTF8);
	// TODO: What do we pass as URL-parameter?
	xmlDoc *doc = xmlReadMemory(data, size, "url", encoding, 0);
	if (!doc)
		return 0;
	timestamp_t res = extract_timestamp(xmlDocGetRootElement(doc));
	xmlFreeDoc(doc);
	return res;
}
