/*
 * lib.h
 * Common definitions and function prototypes.
 * $Id: lib.h 145 2008-11-26 10:05:16Z catseye $
 */

#ifndef	__LIB_H_
#define	__LIB_H_

#ifndef NULL
#define	NULL	0
#endif

#ifndef NDEBUG
#include <stdio.h>
#define	assert(cond) {							\
	if (!(cond)) {							\
		fprintf(stderr,						\
		   "assertion failed (" __FILE__ "): " #cond "\n");	\
		abort();						\
	}								\
}
#else
#define	assert(cond)
#endif

#ifdef STANDALONE
/* stdlib.h */
void	*malloc(unsigned int);
void	 free(void *);
void	 exit(int);
void	 abort(void);

/* string.h */
int	 strcmp(const char *, const char *);
int	 strncmp(const char *, const char *, unsigned int);
char	*strncpy(char *, const char *, unsigned int);
int      strlen(const char *);
void	*memset(void *, int, unsigned int);

/* ctype.h */
int	 k_isspace(char);
int	 k_isdigit(char);
int	 k_isalpha(char);
#else
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#define k_isspace(x) isspace((int)x)
#define k_isdigit(x) isdigit((int)x)
#define k_isalpha(x) isalpha((int)x)
#endif

int	 k_atoi(const char *, unsigned int);

#endif	/* !__LIB_H_ */
