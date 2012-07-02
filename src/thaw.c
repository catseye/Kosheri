/*
 * thaw.c
 * Load a term from a binary termfile and
 * write out a textual version.
 */

#include "lib.h"
#include "cmdline.h"

#include "stream.h"
#include "file.h"

#include "report.h"
#include "value.h"

#include "load.h"
#include "portray.h"

/* Main Program / Driver */

static void
thaw_main(struct value *args, struct value *result)
{
	struct process *p;
        struct reporter *r;
	struct value term;

        struct value *termfile, *binfile;
        struct value termfile_sym, binfile_sym;

        value_symbol_new(&termfile_sym, "termfile", 8);
        value_symbol_new(&binfile_sym, "binfile", 7);
  	termfile = value_dict_fetch(args, &termfile_sym);
	binfile = value_dict_fetch(args, &binfile_sym);

	r = reporter_new("Thawing", NULL, 1);

	/*
	 * Read in.
	 */
	p = file_open(value_symbol_get_token(binfile), "r");
	value_load(&term, p);
	stream_close(NULL, p);

	/*
	 * Write out.
	 */
	p = file_open(value_symbol_get_token(termfile), "w");
	value_portray(p, &term);
	stream_close(NULL, p);

	/*
	 * Finish up.
	 */
	value_integer_set(result, reporter_has_errors(r) ? 1 : 0);
	reporter_free(r);
}

MAIN(thaw_main)
