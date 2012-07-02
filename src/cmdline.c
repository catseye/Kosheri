/*
 * cmdline.c
 * Implementation of common command-line parsing functionality.
 * $Id$
 */

#include "lib.h"
#include "render.h"
#include "process.h"
#include "file.h"

#include "cmdline.h"

struct process	*process_std = NULL;
struct process	*process_err = NULL;

/*
static const char *progname = NULL;
static const struct argdecl *argdecl = NULL;
*/

/* eventually this should parse an optional cmdline schema to validate the cmd line */
/* const struct argdecl *ad,  */

int
cmdline_parse(struct value *dict, int argc, char **argv)
{
    unsigned int arg_count, i;
    struct value name, value;

    assert(argc > 0);
    arg_count = (unsigned int)argc;

    value_dict_new(dict, 16); /* xxx */

    for (i = 1; i < arg_count; i++) {
        if (strlen(argv[i]) > 2 && argv[i][0] == '-' && argv[i][1] == '-') {
            const char *arg_name = argv[i] + (2 * sizeof(char));
            value_symbol_new(&name, arg_name, strlen(arg_name));
            i++;
            if (i < arg_count) {
                value_symbol_new(&value, argv[i], strlen(argv[i]));
                value_dict_store(dict, &name, &value);
            } else {
              /* complain */
            }
        } else {
          /* complain */
        }
    }

    return 1;
}

/* eventually this should parse an optional cmdline schema to validate the cmd line */

void
cmdline_usage(void)
{
    exit(1);
}

int
cmdline_driver(void (*main)(struct value *, struct value *),
               int argc, char **argv)
{
    struct value cmdline, result;

    cmdline_parse(&cmdline, argc, argv);
    assert(value_is_tuple(&cmdline));

    main(&cmdline, &result);

    return value_get_integer(&result);
}
