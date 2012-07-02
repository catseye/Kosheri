/*
 * lib.c
 * Common functions.
 * $Id: lib.c 114 2007-02-27 02:09:02Z catseye $
 */

#include "lib.h"

#ifdef STANDALONE
/*
 * To quote the FreeBSD manpage (despite that this is a reimplementation):
 * The strncpy() function copies not more than len characters from src into
 * dst, appending `\0' characters if src is less than len characters long,
 * and not terminating dst otherwise.
 */
#ifndef USE_SYSTEM_STRNCPY
char *
strncpy(char *dst, const char *src, unsigned int len)
{
	char *r = dst;

	while (*src != '\0' && len > 0) {
		*dst = *src;
		dst++;
		src++;
		len--;
	}
	while (len > 0) {
		/* append \0 characters */
		*dst = '\0';
		dst++;
		len--;
	}

	return r;
}
#endif /* !USE_SYSTEM_STRNCPY */

int
k_isspace(char c)
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

int
k_isdigit(char c)
{
	return c >= '0' && c <= '9';
}

int
k_isalpha(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

#ifndef USE_SYSTEM_STRCMP
int
strcmp(const char *s1, const char *s2)
{
	while (*s1 == *s2 && *s1 != '\0' && *s2 != '\0') {
		s1++;
		s2++;
	}
	if (*s1 == '\0' && *s2 == '\0') {
		return 0;
	}
	if (*s1 > *s2) {
		return 1;
	} else {
		return -1;
	}
}
#endif /* !USE_SYSTEM_STRCMP */

#endif /* STANDALONE */

int
k_atoi(const char *s, unsigned int len)
{
	int acc = 0;

	while (k_isspace(*s) && len > 0) {
		s++;
		len--;
	}
	while (k_isdigit(*s) && len > 0) {
		acc = acc * 10 + (*s - '0');
		s++;
		len--;
	}

	return acc;
}
