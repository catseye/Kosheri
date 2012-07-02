/*
 * gen.c
 * Accumulate values in tuples with extents.
 */

#include "lib.h"

#include "gen.h"

#ifdef DEBUG
#include "cmdline.h"
#include "portray.h"
#include "render.h"
#endif

/*
 * This was originally intended to generate VM instructions, however
 * it has more general applicability.
 */

#define GEN_ROOT_TUPLE		0 /* The top tuple of the hierarchy */
#define GEN_CURRENT_TUPLE	1 /* The tuple currently being gen'ed into */
#define GEN_GLOBAL_POS		2 /* The global position to be gen'ed into */
#define GEN_LOCAL_POS		3 /* Gen offset into current tuple */

#define GEN_SIZE		4

int
gen_new(struct value *gen, struct value *tuple, unsigned int pos)
{
	struct value gen_tag;

	value_symbol_new(&gen_tag, "gen", 3);
	if (!value_tuple_new(gen, &gen_tag, GEN_SIZE))
		return 0;
	value_tuple_store(gen, GEN_ROOT_TUPLE, tuple);
	value_tuple_store(gen, GEN_CURRENT_TUPLE, tuple);
	value_tuple_store_integer(gen, GEN_GLOBAL_POS, pos);
	value_tuple_store_integer(gen, GEN_LOCAL_POS, pos);
	return 1;
}

int
gen_new_default(struct value *gen)
{
	struct value tuple, code_sym;

	value_symbol_new(&code_sym, "code", 4);
	value_tuple_new(&tuple, &code_sym, 8192);
	return gen_new(gen, &tuple, 0);
}

/*
 * Accumulate a value to a tuple.
 */
void
gen_value(struct value *gen, struct value *value)
{
	unsigned int pos = value_tuple_fetch_integer(gen, GEN_LOCAL_POS);
	struct value *tuple = value_tuple_fetch(gen, GEN_CURRENT_TUPLE);
	unsigned int size = value_tuple_get_size(tuple);
	struct value new_tuple;

	if (pos == (size - 1)) {
		/* Last slot.  Allocate a new tuple of twice the running size and plug it in. */
		struct value *tag = value_tuple_get_tag(tuple);

		value_tuple_new(&new_tuple, tag, size * 2);
		value_tuple_store(tuple, pos, &new_tuple);
		value_tuple_store(gen, GEN_CURRENT_TUPLE, &new_tuple);
		pos = 0;
		tuple = &new_tuple;
	}
	value_tuple_store(tuple, pos, value);
	pos++;
	value_tuple_store_integer(gen, GEN_LOCAL_POS, pos);

	pos = value_tuple_fetch_integer(gen, GEN_GLOBAL_POS);
	pos++;
	value_tuple_store_integer(gen, GEN_GLOBAL_POS, pos);
}

/*
 * Accumulate a value to a tuple.
 */
void
gen_integer(struct value *gen, int i)
{
	struct value val;

	value_integer_set(&val, i);
	gen_value(gen, &val);
}

/*
 * Labels - forward reference and backpatching mechanism.
 * This is designed to be used internally.  External clients
 * (that is, where labels must be human-readable, such as the
 * assembler) should maintain a dictionary associating strings
 * with labels.
 *
 * Use cases:
 *
 * 1) Backward reference (requires no backpatching):
 *
 *      label = gen_define_label(gen, NULL);
 *      ...
 *      gen_integer(gen, INSTR_GOTO);
 *      gen_gen_label_ref(gen, label);
 *
 * 2) Forward reference (requires backpatching):
 *
 *      gen_integer(gen, INSTR_GOTO);
 *      label = gen_gen_label_ref(gen, NULL);
 *      ...
 *      gen_define_label(gen, label);
 *
 */

#define GEN_LABEL_TUPLE		0 /* Tuple into which the label points */
#define GEN_LABEL_LOCAL_POS	1 /* local pos to which label refers */
#define GEN_LABEL_GLOBAL_POS	2 /* global pos to which label refers */
#define GEN_LABEL_NEXT		3 /* list of backpatches to apply */

#define GEN_LABEL_SIZE		4

static int
gen_label_new(struct value *gen_label)
{
	struct value gen_label_tag;

	value_symbol_new(&gen_label_tag, "genlab", 3);
	if (!value_tuple_new(gen_label, &gen_label_tag, GEN_LABEL_SIZE))
		return 0;
	value_tuple_store(gen_label, GEN_LABEL_TUPLE, &VNULL);
	value_tuple_store_integer(gen_label, GEN_LABEL_LOCAL_POS, 0);
	value_tuple_store_integer(gen_label, GEN_LABEL_GLOBAL_POS, 0);
	value_tuple_store(gen_label, GEN_LABEL_NEXT, &VNULL);

	return 1;
}

/*
 * Generate a reference to a label into the tuple, for instance as the
 * immediate argument of a branch instruction.  If the label parameter
 * is NULL, this will generate and return a forward reference which
 * should be resolved by subsequently passing it to gen_define_label().
 */
void
gen_gen_label_ref(struct value *gen, struct value *gen_label)
{
	struct value *bp;
	struct value next;
	int global_pos;

	assert(gen_label != NULL);

	if (value_is_null(gen_label)) {
		/* Not yet allocated, so allocate a new undefined one. */
		gen_label_new(gen_label);
	}

	global_pos = value_tuple_fetch_integer(gen_label, GEN_LABEL_GLOBAL_POS);
	if (global_pos != 0) {
		/* Already defined, so just use it. */
		gen_integer(gen, global_pos);
		return;
	}

	/*
	 * The label is newly allocated, or at least has not been defined
	 * yet.  So, we remember that we will need to backpatch here in the
	 * future (by adding an entry to the label's backpatch list) and,
	 * for now, generate a NULL in its slot.
	 */

	value_copy(&next, value_tuple_fetch(gen_label, GEN_LABEL_NEXT));
	bp = value_tuple_fetch(gen_label, GEN_LABEL_NEXT);
	gen_label_new(bp);
	value_tuple_store(bp, GEN_LABEL_TUPLE, value_tuple_fetch(gen, GEN_CURRENT_TUPLE));
	value_tuple_store(bp, GEN_LABEL_LOCAL_POS, value_tuple_fetch(gen, GEN_LOCAL_POS));
	value_tuple_store(bp, GEN_LABEL_GLOBAL_POS, value_tuple_fetch(gen, GEN_GLOBAL_POS));
	value_tuple_store(bp, GEN_LABEL_NEXT, &next);

	gen_value(gen, &VNULL);
}

/*
 * Define a label.  If the label is already allocated, this will
 * cause previously-generated forward references to this label to be
 * backpatched with the now-known location.
 */
int
gen_define_label(struct value *gen, struct value *gen_label)
{
	struct value *tuple = value_tuple_fetch(gen, GEN_CURRENT_TUPLE);
	unsigned int local_pos = value_tuple_fetch_integer(gen, GEN_LOCAL_POS);
	unsigned int global_pos = value_tuple_fetch_integer(gen, GEN_GLOBAL_POS);

	struct value *bp;

	assert(gen_label != NULL);

	if (value_is_null(gen_label)) {
		/* Not yet allocated, so allocate a new undefined one. */
		gen_label_new(gen_label);
	} else {
		/* Fail if we try to redefine a label. */
		if (value_tuple_fetch_integer(gen_label, GEN_LABEL_GLOBAL_POS) != 0) {
			return 0;
		}
	}

	value_tuple_store(gen_label, GEN_LABEL_TUPLE, tuple);
	value_tuple_store_integer(gen_label, GEN_LABEL_LOCAL_POS, local_pos);
	value_tuple_store_integer(gen_label, GEN_LABEL_GLOBAL_POS, global_pos);

	/*
	 * Resolve any previous forward references by backpatching.
	 */
	bp = value_tuple_fetch(gen_label, GEN_LABEL_NEXT);
	while (!value_is_null(bp)) {
		/*
		 * Backpatch, by placing the current global position into
		 * the slot named by the bp entry, then remove entry from list.
		 */
		value_tuple_store_integer(
		    value_tuple_fetch(bp, GEN_LABEL_TUPLE),
		    value_tuple_fetch_integer(bp, GEN_LABEL_LOCAL_POS),
		    global_pos
		);
		value_tuple_store(gen_label, GEN_LABEL_NEXT,
		    value_tuple_fetch(bp, GEN_LABEL_NEXT));
		bp = value_tuple_fetch(gen_label, GEN_LABEL_NEXT);
	}

	return 1;
}

/*
 * Return a single tuple containing the contents of the gen'ed tuple chain.
 */
void
gen_flatten(struct value *gen, struct value *dest)
{
	unsigned int dest_size = value_tuple_fetch_integer(gen, GEN_GLOBAL_POS);
	struct value *current = value_tuple_fetch(gen, GEN_ROOT_TUPLE);
	unsigned int j = 0;

	value_tuple_new(dest, value_tuple_get_tag(current), dest_size);

	while (!value_is_null(current)) {
		unsigned int i = 0;
		unsigned int current_size = value_tuple_get_size(current);
		struct value *x;
		for (i = 0; i < (current_size - 1) && i < dest_size; i++) {
			x = value_tuple_fetch(current, i);

#ifdef DEBUG
			process_render(process_err, "(flatten %d/%d -> %d/%d: ", i, current_size, j, dest_size);
			value_portray(process_err, x);
			process_render(process_err, ")\n");
#endif

			value_tuple_store(dest, j, x);
			j++;
		}
		current = value_tuple_fetch(current, (current_size - 1));
	}
}
