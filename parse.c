#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

/*
 * File boundaries are dive boundaries. But sometimes there are
 * multiple dives per file, so there can be other events too that
 * trigger a "new dive" marker and you may get some nesting due
 * to that. Just ignore nesting levels.
 */
static void dive_start(void)
{
	printf("---\n");
}

static void dive_end(void)
{
}

static void sample_start(void)
{
	printf("Sample:\n");
}

static void sample_end(void)
{
}

static void entry(const char *name, int size, const char *buffer)
{
	printf("%s: %.*s\n", name, size, buffer);
}

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

static void visit_one_node(xmlNode *node)
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

	entry(name, len, content);
}

static void traverse(xmlNode *node)
{
	xmlNode *n;

	for (n = node; n; n = n->next) {
		/* XML from libdivecomputer: 'dive' per new dive */
		if (!strcmp(n->name, "dive")) {
			dive_start();
			traverse(n->children);
			dive_end();
			continue;
		}

		/*
		 * At least both libdivecomputer and Suunto
		 * agree on "sample".
		 *
		 * Well - almost. Ignore case.
		 */
		if (!strcasecmp(n->name, "sample")) {
			sample_start();
			traverse(n->children);
			sample_end();
			continue;
		}

		/* Anything else - just visit it and recurse */
		visit_one_node(n);
		traverse(n->children);
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

	dive_start();
	traverse(xmlDocGetRootElement(doc));
	dive_end();
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
