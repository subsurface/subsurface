#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dive.h"

static void show_dive(int nr, struct dive *dive)
{
	int i;
	struct tm *tm;

	tm = gmtime(&dive->when);

	printf("At %02d:%02d:%02d %04d-%02d-%02d  (%d ft max, %d minutes)\n",
		tm->tm_hour, tm->tm_min, tm->tm_sec,
		tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
		to_feet(dive->maxdepth), dive->duration.seconds / 60);

	if (!verbose)
		return;

	for (i = 0; i < dive->samples; i++) {
		struct sample *s = dive->sample + i;

		printf("%4d:%02d: %3d ft, %2d C, %4d PSI\n",
			s->time.seconds / 60,
			s->time.seconds % 60,
			to_feet(s->depth),
			to_C(s->temperature),
			to_PSI(s->tankpressure));
	}
}

static int sortfn(const void *_a, const void *_b)
{
	const struct dive *a = *(void **)_a;
	const struct dive *b = *(void **)_b;

	if (a->when < b->when)
		return -1;
	if (a->when > b->when)
		return 1;
	return 0;
}

static void report_dives(void)
{
	int i;

	qsort(dive_table.dives, dive_table.nr, sizeof(struct dive *), sortfn);
	for (i = 0; i < dive_table.nr; i++)
		show_dive(i+1, dive_table.dives[i]);
}

static void parse_argument(const char *arg)
{
	const char *p = arg+1;

	do {
		switch (*p) {
		case 'v':
			verbose++;
			continue;
		default:
			fprintf(stderr, "Bad argument '%s'\n", arg);
			exit(1);
		}
	} while (*++p);
}

int main(int argc, char **argv)
{
	int i;

	parse_xml_init();

	for (i = 1; i < argc; i++) {
		const char *a = argv[i];

		if (a[0] == '-') {
			parse_argument(a);
			continue;
		}
		parse_xml_file(a);
	}
	report_dives();
	return 0;
}

