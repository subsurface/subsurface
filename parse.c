#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

static void show_one_node(int i, xmlNode *node)
{
	static const char indent[] = "        ..";

	if (i >= sizeof(indent))
		i = sizeof(indent)-1;
	printf("%.*snode '%s': %s\n", i, indent, node->name, node->content);
}

static void show(int indent, xmlNode *node)
{
	xmlNode *n;

	for (n = node; n; n = n->next) {
		show_one_node(indent, n);
		show(indent+2, n->children);
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

	show(0, xmlDocGetRootElement(doc));
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
