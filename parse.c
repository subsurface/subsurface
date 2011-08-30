#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

static const char *nodename(xmlNode *node, char *buf, int len)
{
	/* Don't print out the node name if it is "text" */
	if (!strcmp(node->name, "text")) {
		node = node->parent;
		if (!node || !node->name)
			return "root";
	}

	buf += len;
	*--buf = 0;
	len--;

	for(;;) {
		const char *name = node->name;
		int i = strlen(name);
		while (--i >= 0) {
			unsigned char c = name[i];
			*--buf = tolower(c);
			if (!--len)
				return buf;
		}
		node = node->parent;
		if (!node || !node->name)
			return buf;
		*--buf = '.';
		if (!--len)
			return buf;
	}
}

#define MAXNAME 64

static void show_one_node(xmlNode *node)
{
	int len;
	const unsigned char *content;
	char buffer[MAXNAME];
	const char *name;

	content = node->content;
	if (!content)
		return;

	/* Trim whitespace at beginning */
	while (isspace(*content))
		content++;

	/* Trim whitespace at end */
	len = strlen(content);
	while (len && isspace(content[len-1]))
		len--;

	if (!len)
		return;

	name = nodename(node, buffer, sizeof(buffer));

	printf("%s: %.*s\n", name, len, content);
}

static void show(xmlNode *node)
{
	xmlNode *n;

	for (n = node; n; n = n->next) {
		show_one_node(n);
		show(n->children);
	}
}

static void parse(const char *filename)
{
	xmlDoc *doc;

	doc = xmlReadFile(filename, NULL, 0);
	if (!doc) {
		fprintf(stderr, "Failed to parse '%s'.\n", filename);
		return;
	}

	show(xmlDocGetRootElement(doc));
	xmlFreeDoc(doc);
	xmlCleanupParser();
}

int main(int argc, char **argv)
{
	int i;

	LIBXML_TEST_VERSION

	for (i = 1; i < argc; i++)
		parse(argv[i]);
	return 0;
}
