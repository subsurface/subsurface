#ifndef STRNDUP_H
#define STRNDUP_H
#if __WIN32__
static char *strndup (const char *s, size_t n)
{
	char *cpy;
	size_t len = strlen(s);
	if (n < len)
		len = n;
	if ((cpy = malloc(len + 1)) !=
	    NULL) {
		cpy[len] =
				'\0';
		memcpy(cpy,
		       s,
		       len);
	}
	return cpy;
}
#endif
#endif /* STRNDUP_H */
