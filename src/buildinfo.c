/*
 * buildinfo.c
 * Show some info about the built binary (also test for process-based I/O)
 */

#include "lib.h"

#include "render.h"
#include "cmdline.h"

#include "process.h"
#include "file.h"

/* Main Program / Driver */

static void
buildinfo_main(struct value *args, struct value *result)
{
	struct process *out;

	out = file_open("*stdout", "w");
	args = args;
	process_render(out,
	    "sizeof(struct value) == %d\n", sizeof(struct value));
        value_integer_set(result, 0);
}

MAIN(buildinfo_main)
