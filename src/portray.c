/*
 * portray.c
 * Routines for parsing and rendering values.
 * $Id: portray.c 143 2008-07-18 06:42:22Z catseye $
 */

#include "lib.h"

#include "value.h"
#include "portray.h"
#include "render.h"

static void value_portray_nodups(struct process *, struct value *, struct value *);

/*
 * Portray a value (i.e. render it in a human-readable way) to a process.
 */
void
value_portray(struct process *p, struct value *v)
{
        struct value seen;
        value_dict_new(&seen, 31);
	value_portray_nodups(p, v, &seen);
}

/*
 * Recursive portion of value_portray().  Tracks a dictionary of all
 * tuple values that have been seen, to avoid recursing infinitely
 * into cyclic chains of nested tuples.
 */
static void
value_portray_nodups(struct process *p, struct value *v, struct value *seen)
{
	switch (v->type) {
	case VALUE_NULL:
		process_render(p, "[]");
		break;
	case VALUE_INTEGER:
		process_render(p, "%d", value_get_integer(v));
		break;
	case VALUE_BOOLEAN:
		process_render(p, "%s", value_get_boolean(v) ? "true" : "false");
		break;
	case VALUE_PROCESS:
		process_render(p, "PROCESS#[0x%08x]", (long int)value_get_process(v));
		break;
	case VALUE_LABEL:
		process_render(p, "LABEL#[0x%08x]", (long int)value_get_label(v));
		break;
	case VALUE_SYMBOL:
		process_render(p, "%s", value_symbol_get_token(v));
		break;
	case VALUE_TUPLE:
	    {
                struct value *tag = value_tuple_get_tag(v);
		unsigned int max = value_tuple_get_size(v);
		unsigned int i;

                if (!value_is_null(seen))
                        value_dict_store(seen, v, &VTRUE);

                /* XXX should eventually dispatch to a handler based on tag. */
                if (value_equal(tag, &tag_dict)) {
                        struct value dict_iter;
                        struct value *key;

                        value_dict_new_iter(&dict_iter, v);

                        process_render(p, "{");
                        key = value_dict_iter_get_current_key(&dict_iter);
                        while (!value_is_null(key)) {
                                value_portray_nodups(p, key, seen);
                                process_render(p, "=");
                                /* XXX not so good; use iter */
                                value_portray_nodups(p, value_dict_fetch(v, key), seen);
                                value_dict_iter_advance(&dict_iter);
                                key = value_dict_iter_get_current_key(&dict_iter);
                                if (!value_is_null(key)) {
                                        process_render(p, ", ");
                                }
                        }
                        process_render(p, "}");
                } else {
                        process_render(p, "<");
                        value_portray(p, value_tuple_get_tag(v));
                        process_render(p, ": ");
                        for (i = 0; i < max; i++) {
                                struct value *k = value_tuple_fetch(v, i);
                                if (value_is_null(seen) ||
                                    value_is_null(value_dict_fetch(seen, k))) {
                                        value_portray_nodups(p, k, seen);
                                } else {
                                        process_render(p, "TUPLE#[0x%08x]",
						value_get_unique_id(v));
                                }
                                if (i < (max - 1))
                                        process_render(p, ", ");
                        }
                        process_render(p, ">");
                }
		break;
	    }
	}
}
