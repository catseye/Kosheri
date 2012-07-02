/*
 * report.c
 * Error/warning reporter.
 * $Id: report.c 139 2008-07-16 09:56:31Z catseye $
 */

#include <stdarg.h>

#include "lib.h"

#include "process.h"
#include "stream.h"
#include "file.h"

#include "render.h"

#include "report.h"

struct reporter {
        int		 finished;
        int		 verbose;
        int		 errors;
        int		 warnings;
        struct process	*stream;
        const char	*phase;
};

/*
 * phase should always point to a literal constant string like "Parsing"
 */
struct reporter *
reporter_new(const char *phase, struct process *stream, int verbose)
{
	struct reporter *r;

	if ((r = malloc(sizeof(struct reporter))) == NULL) {
		return NULL;
	}

	r->verbose = verbose;
        r->errors = 0;
        r->warnings = 0;
        r->finished = 0;
        r->stream = stream;
	if (r->stream == NULL) {
		r->stream = file_open("*stderr", "w");
	}
        r->phase = phase;

	return r;

	if (verbose) {
		process_render(r->stream, "%s started\n", r->phase);
	}
}

void
reporter_free(struct reporter *r)
{
        if (!r->finished) {
                reporter_finish(r);
        }
        stream_close(NULL, r->stream);
	free(r);
}

int
reporter_finish(struct reporter *r)
{
	if (r->verbose) {
		process_render(r->stream,
		    "%s finished with %d errors and %d warnings\n",
		    r->phase, r->errors, r->warnings);
	}
	r->phase = NULL;
        r->finished = 1;
	return r->errors;
}

struct process *
reporter_stream(struct reporter *r)
{
        return r->stream;
}

int
reporter_has_errors(struct reporter *r)
{
	return r->errors > 0;
}

void
report(struct reporter *r, enum report_type rtype, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
        report_va_list(r, rtype, fmt, args);
        va_end(args);
}

void
report_va_list(struct reporter *r, enum report_type rtype, const char *fmt, va_list args)
{
	int i;

        assert(!r->finished);

        process_render(r->stream, "%s %s: ",
            r->phase, rtype == REPORT_ERROR ? "Error" : "Warning");

	for (i = 0; fmt[i] != '\0'; i++) {
		if (fmt[i] == '%') {
			i++;
			switch (fmt[i]) {
			case 's':
				process_render(r->stream, "%s",
				    va_arg(args, char *));
				break;
			case 'd':
				process_render(r->stream, "%d",
				    va_arg(args, int));
				break;
			}
		} else {
			process_render(r->stream, "%c", fmt[i]);
		}
	}

	process_render(r->stream, ".\n");

	switch (rtype) {
	case REPORT_ERROR:
		r->errors++;
		break;
	case REPORT_WARNING:
		r->warnings++;
		break;
	}
}
