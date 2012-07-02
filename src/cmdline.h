/*
 * cmdline.h
 * Prototypes for common command-line parsing functionality.
 * $Id$
 */

/*
 * In the future, this should be progenv.h, which determines a
 * program environment.  This should include an "entry point"
 * (not necessarily "main", since it should be able to operate
 * in standalone and other contexts where a "command line" is
 * not meaningful.)
 */

#ifndef __CMDLINE_H_
#define __CMDLINE_H_

#include "value.h"

#include "file.h"   /* ... because. */

struct process;

struct runenv {
        struct handler	     *handler_std;
	/*
        struct stream	     *stream_std;
        struct stream	     *stream_err;
	*/
        const char           *progname;
};

extern struct process	*process_err;

int             cmdline_driver(void (*)(struct value *, struct value *),
                               int, char **);
int             cmdline_parse(struct value *, int, char **);
void            cmdline_usage(void);

/*
 * STANDALONE no longer supported / not yet re-supported
 */
#ifdef STANDALONE
#define	MAIN(function)							\
	int main(int argc, char **argv)					\
	{								\
		process_err = NULL;					\
                return cmdline_driver(function, argc, argv);		\
	}
#else
#define	MAIN(function)							\
	int main(int argc, char **argv)					\
	{								\
		process_err = file_open("*stderr", "w");		\
                return cmdline_driver(function, argc, argv);		\
	}
#endif	/* STANDALONE */

#endif	/* !__CMDLINE_H_ */
