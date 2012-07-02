/*
 * freeze.c
 * Parse a constant term from a text file and
 * write it out to a binary termfile.
 * $Id: freeze.c 146 2008-12-06 19:59:54Z catseye $
 */

#include "lib.h"
#include "cmdline.h"

#include "stream.h"
#include "file.h"

#include "report.h"
#include "value.h"

#include "discern.h"
#include "scan.h"
#include "save.h"

/* Main Program / Driver */

static void
freeze_main(struct value *args, struct value *result)
{
	struct process *p;
	struct scanner *sc;
        struct reporter *r;
	struct value term;

        struct value *termfile, *binfile;
        struct value termfile_sym, binfile_sym;

        value_symbol_new(&termfile_sym, "termfile", 8);
        value_symbol_new(&binfile_sym, "binfile", 7);
  	termfile = value_dict_fetch(args, &termfile_sym);
	binfile = value_dict_fetch(args, &binfile_sym);

	r = reporter_new("Freezing", NULL, 1);

	/*
	 * Parse.
	 */
	sc = scanner_new(r);
	if (!scanner_open(sc, value_symbol_get_token(termfile))) {
                value_integer_set(result, 1);
                return;
        }
	if (!value_discern(&term, sc)) {
		report(r, REPORT_ERROR, "Could not parse input term");
        }

	/*
	 * Write out.
	 */
	p = file_open(value_symbol_get_token(binfile), "w");
	if (!value_save(p, &term)) {
		report(r, REPORT_ERROR,
		   "Could not write term to '%s'", binfile);
	}
	stream_close(NULL, p);

	/*
	 * Finish up.
	 */
	scanner_close(sc);
	scanner_free(sc);

	value_integer_set(result, reporter_has_errors(r) ? 1 : 0);
	reporter_free(r);
}

MAIN(freeze_main)
