/*
 * value.c
 * Values.
 */

#include "lib.h"

#include "value.h"

#ifdef DEBUG
#include "cmdline.h"
#include "portray.h"
#include "render.h"
#endif

/*
 * Structured values - strings, function values, and the like.
 * These are dynamically allocated, garbage collected, and so forth.
 */
struct structured_value {
	unsigned char		 admin;		/* ADMIN_ flags */
	struct structured_value	*next;
};

#define	ADMIN_FREE		1	/* on the free list */
#define	ADMIN_MARKED		2	/* marked, during gc */

struct value VNULL = { VALUE_NULL, { 0 } };

struct value VFALSE = { VALUE_BOOLEAN, { 0 } };
struct value VTRUE = { VALUE_BOOLEAN, { 1 } };

struct value tag_vm = { VALUE_INTEGER, { 1 } };
struct value tag_ar = { VALUE_INTEGER, { 3 } };
struct value tag_dict = { VALUE_INTEGER, { 4 } };
struct value tag_list = { VALUE_INTEGER, { 5 } };
struct value tag_iter = { VALUE_INTEGER, { 6 } };

#ifdef DEBUG
const char *type_name_table[] = {
	"NULL",
	"INTEGER",
	"BOOLEAN",
	"PROCESS",
	"LABEL",
	"???5???",
	"???6???",
	"???7???",
	"???8???",
	"SYMBOL",
	"TUPLE"
};
#endif

/*
 * List of structured values; used for sweep phase of GC.
 */
static struct structured_value *sv_head = NULL;

/*** unstructured values ***/

void
value_integer_set(struct value *v, int i)
{
	v->type = VALUE_INTEGER;
	v->value.integer = i;
}

void
value_boolean_set(struct value *v, int b)
{
	v->type = VALUE_BOOLEAN;
	v->value.boolean = b;
}

void
value_process_set(struct value *v, struct process *p)
{
	v->type = VALUE_PROCESS;
	v->value.process = p;
}

void
value_label_set(struct value *v, clabel l)
{
	v->type = VALUE_LABEL;
	v->value.label = l;
}

/*** structured values ***/

/*
 * Initialize a structured value by link it up into the
 * garbage-collection list.
 */
static void
structured_value_init(struct structured_value *sv)
{
	sv->admin = 0;
	sv->next = sv_head;
	sv_head = sv;
}

/***** symbols *****/

struct symbol {
	struct structured_value	sv;
	unsigned int		length;	    /* number of characters in symbol */
     /* char			token[]; */ /* lexeme of this symbol */
};

int
value_symbol_new(struct value *v, const char *token, unsigned int len)
{
	char *buffer;

	assert(token != NULL);

	buffer = value_symbol_new_buffer(v, len);
	if (buffer == NULL)
		return 0;
	strncpy(buffer, token, len);

	return 1;
}

char *
value_symbol_new_buffer(struct value *v, unsigned int len)
{
	struct symbol *sym;

	assert(v != NULL);

	sym = malloc(sizeof(struct symbol) + len + 1);
	if (sym == NULL)
		return NULL;
	sym->length = len;
	((char *)(sym + 1))[len] = '\0';

	v->type = VALUE_SYMBOL;
	v->value.structured = (struct structured_value *)sym;
	structured_value_init((struct structured_value *)sym);

	return (char *)(sym + 1);
}


/*
 * Returned string is NUL-terminated.
 */
const char *
value_symbol_get_token(const struct value *v)
{
	struct symbol *sym;

	assert(v->type == VALUE_SYMBOL);
	assert(v->value.structured != NULL);
	sym = (struct symbol *)v->value.structured;
	return ((const char *)(sym + 1));
}

unsigned int
value_symbol_get_length(const struct value *v)
{
	struct symbol *sym;

	assert(v->type == VALUE_SYMBOL);
	assert(v->value.structured != NULL);
	sym = (struct symbol *)v->value.structured;
	return sym->length;
}

/***** tuples *****/

struct tuple {
	struct structured_value sv;
	struct value		tag;
	unsigned int		size;	/* in # of values contained */
	/* struct value		vector[]; */
};

int
value_tuple_new(struct value *v, struct value *tag, unsigned int size)
{
	struct tuple *tuple;

	unsigned int bytes = sizeof(struct tuple) +
			     sizeof(struct value) * size;

	if ((tuple = malloc(bytes)) == NULL)
		return 0;

	memset(tuple, 0, bytes);
	value_copy(&tuple->tag, tag);
	tuple->size = size;

	v->type = VALUE_TUPLE;
	v->value.structured = (struct structured_value *)tuple;
	structured_value_init((struct structured_value *)tuple);

	return 1;
}

int
value_is_tuple(const struct value *v)
{
	return v->type == VALUE_TUPLE;
}

static struct tuple *
value_get_tuple(const struct value *v)
{
	assert(value_is_tuple(v));
	assert(v->value.structured != NULL);
	return (struct tuple *)v->value.structured;
}

struct value *
value_tuple_get_tag(const struct value *v)
{
	struct tuple *t = value_get_tuple(v);
	return &t->tag;
}

unsigned int
value_tuple_get_size(const struct value *v)
{
	struct tuple *t = value_get_tuple(v);
	return t->size;
}

struct value *
value_tuple_fetch(const struct value *v, unsigned int at)
{
	struct tuple *t = value_get_tuple(v);
	assert(at < t->size);
	return (struct value *)(t + 1) + at;
}

void
value_tuple_store(struct value *v, unsigned int at, const struct value *src)
{
	struct value *dst = value_tuple_fetch(v, at);

	value_copy(dst, src);
}

int
value_tuple_fetch_integer(const struct value *v, unsigned int at)
{
	struct tuple *t = value_get_tuple(v);
	assert(at < t->size);
	assert(((struct value *)(t + 1) + at)->type == VALUE_INTEGER);
	return ((struct value *)(t + 1) + at)->value.integer;
}

clabel
value_tuple_fetch_label(const struct value *v, unsigned int at)
{
	struct tuple *t = value_get_tuple(v);
	assert(at < t->size);
	assert(((struct value *)(t + 1) + at)->type == VALUE_LABEL);
	return ((struct value *)(t + 1) + at)->value.label;
}

/*
 * Convenience method to directly store an integer in a tuple.
 */
void
value_tuple_store_integer(struct value *v, unsigned int at, int src)
{
	struct value *dst = value_tuple_fetch(v, at);

	value_integer_set(dst, src);
}

/*** ACCESSORS ***/

/* Unstructured values */

int
value_get_integer(const struct value *v)
{
	assert(v->type == VALUE_INTEGER);
	return v->value.integer;
}

int
value_is_integer(const struct value *v)
{
	return v->type == VALUE_INTEGER;
}

int
value_get_boolean(const struct value *v)
{
	assert(v->type == VALUE_BOOLEAN);
	return v->value.boolean;
}

struct process *
value_get_process(const struct value *v)
{
	assert(v->type == VALUE_PROCESS);
	return v->value.process;
}

clabel
value_get_label(const struct value *v)
{
	assert(v->type == VALUE_LABEL);
	return v->value.label;
}

PTR_INT
value_get_unique_id(const struct value *v)
{
	assert(v->type & VALUE_STRUCTURED);
	return (PTR_INT)v->value.structured;
}


/* Tuples as activation records */

int
value_ar_new(struct value *v, unsigned int size,
	     struct value *caller, struct value *enclosing, unsigned int pc)
{
	if (!value_tuple_new(v, &tag_ar, size + AR_HEADER_SIZE))
		return 0;

	value_tuple_store(v, AR_CALLER, caller);
	value_tuple_store(v, AR_ENCLOSING, enclosing);
	value_tuple_store_integer(v, AR_PC, pc);
	value_tuple_store_integer(v, AR_TOP, AR_HEADER_SIZE);

	return 1;
}

struct value *
value_ar_pop(struct value *ar)
{
	int idx;
	struct value *v;

	idx = value_tuple_fetch_integer(ar, AR_TOP);
	v = value_tuple_fetch(ar, --idx);
	value_tuple_store_integer(ar, AR_TOP, idx);

	return v;
}

void
value_ar_push(struct value *ar, struct value *v)
{
	int idx;

	assert(value_is_tuple(ar));
	idx = value_tuple_fetch_integer(ar, AR_TOP);
	value_tuple_store(ar, idx++, v);
	value_tuple_store_integer(ar, AR_TOP, idx);
}

void
value_ar_xfer(struct value *from, struct value *to, int count)
{
	int i;
	int from_top = value_tuple_fetch_integer(from, AR_TOP);
	int to_top = value_tuple_fetch_integer(to, AR_TOP);

	for (i = 0; i < count; i++) {
		value_tuple_store(to, to_top + i,
		    value_tuple_fetch(from, (from_top - count) + i));
	}

	value_tuple_store_integer(to, AR_TOP, to_top + count);
	value_tuple_store_integer(from, AR_TOP, from_top - count);
}

/***** tuples as dictionaries *****/

/*
 * Dictionaries are implemented with "layered" hash tables.
 * A layered hash table works like so:
 *
 * Instead of N chains of buckets, there are arrays of size N, called
 * 'layers'.  Each layer is chained together.  This is wasteful in
 * space when there are only a few entries AND they hash-collide, since
 * there may be one (or rarely, more) layers which are mostly empty.
 * BUT, this is possibly not as bad as it sounds:
 *
 * - When there are fewer than N entries, and there are no collisions,
 *   space usage is less than the chained version - because the chained
 *   version needs an array of N bucket-heads which is the same size as
 *   a layer - while it also needs buckets.
 * - For M * N entries with no collisions, there are M layers allocated,
 *   and only 2 * M pointers; in the chained version, there are M * N
 *   (next-bucket) pointers allocated.
 * - Allocations for large items (layers) happen less often than
 *   allocations for small items (buckets,) so this may be an advantage
 *   if malloc() is on the slow side.
 */

/*
 * Each layer of a dictionary is represented by a tuple with the
 * following components:
 *
 *   <usage-count, next-layer, ...>
 *
 * where ... represents the actual entries.
 */

#define LAYER_USAGE		0
#define LAYER_NEXT		1
#define LAYER_HEADER_SIZE	2

/*
 * Compute the hash value of the given value.
 */
static unsigned int
value_hash(const struct value *v)
{
	switch (v->type) {
	case VALUE_NULL:
		return 0;
	case VALUE_INTEGER:
		return (unsigned int)v->value.integer;
	case VALUE_BOOLEAN:
		return (unsigned int)v->value.boolean;
	case VALUE_PROCESS:
		/* XXX not stable between load/save */
		return (PTR_INT)v->value.process;
	case VALUE_LABEL:
		/* XXX not stable between load/save */
		return (PTR_INT)v->value.label;
	case VALUE_SYMBOL:
	    {
		unsigned int i, hash_val = 0, len = value_symbol_get_length(v);
		const char *str = value_symbol_get_token(v);

		for (i = 0; i < len; i++) {
			hash_val += str[i] << (i & 0xf);
		}

		return hash_val;
	    }
	case VALUE_TUPLE:
		/* XXX not stable between load/save */
		return (PTR_INT)v->value.structured;
	}
	/* should never be reached */
	assert(v->type == VALUE_NULL);
	return 0;
}

int
value_dict_new(struct value *table, unsigned int layer_size)
{
	if (!value_tuple_new(table, &tag_dict,
			     layer_size + LAYER_HEADER_SIZE)) {
		return 0;
	}

	value_tuple_store_integer(table, LAYER_USAGE, 0);
	value_tuple_store(table, LAYER_NEXT, &VNULL);

	return 1;
}

static struct value *
value_fetch_hash(const struct value *dict, const struct value *key,
                 unsigned int (*hash_fn)(const struct value *))
{
	const struct value *layer;
	unsigned int layer_size;
	unsigned int slot;

	layer = dict;
	layer_size = value_tuple_get_size(layer) - LAYER_HEADER_SIZE;
	slot = (hash_fn(key) % (layer_size >> 1)) << 1;
	slot += LAYER_HEADER_SIZE; /* skip over administrivia in tuple */

	while (!value_is_null(layer)) {
		if (value_equal(key, value_tuple_fetch(layer, slot)))
			return value_tuple_fetch(layer, slot + 1);
		layer = value_tuple_fetch(layer, LAYER_NEXT);
		assert(value_is_null(layer) || value_is_tuple(layer));
	}

	return &VNULL;
}

struct value *
value_dict_fetch(const struct value *dict, const struct value *key)
{
	assert(value_is_tuple(dict));
	return value_fetch_hash(dict, key, value_hash);
}

static void
value_store_hash(struct value *dict, struct value *key, struct value *value,
		 unsigned int (*hash_fn)(const struct value *))
{
	struct value *layer = dict;
	struct value *prev_layer = &VNULL;
	struct value *next_layer = &VNULL;
	struct value new_layer;
	unsigned int layer_size = value_tuple_get_size(layer) - LAYER_HEADER_SIZE;
	unsigned int slot = (hash_fn(key) % (layer_size >> 1)) << 1;
	int usage;
	int delta = 0;

	slot += LAYER_HEADER_SIZE; /* skip over administrivia in tuple */

	for (;;) {
		struct value *v = value_tuple_fetch(layer, slot);
		if (value_is_null(v)) {
			/*
			 * Okay to insert into empty slot, so
			 * just break out of the loop.
			 */
			if (!value_is_null(value)) {
				delta = 1;	/* insert */
			}
			break;
		}
		if (value_equal(key, v)) {
			/*
			 * Okay to overwrite, so
			 * just break out of the loop.
			 */
			if (value_is_null(value)) {
				delta = -1;	/* delete */
			}
			break;
		}
		prev_layer = layer;
		next_layer = value_tuple_fetch(layer, LAYER_NEXT);
		assert(value_is_null(layer) || value_is_tuple(layer));
		if (!value_is_null(next_layer)) {
			layer = next_layer;
			/* and try again */
			continue;
		} else {
			value_dict_new(&new_layer, layer_size);
			assert(value_is_tuple(&new_layer));
			value_tuple_store(layer, LAYER_NEXT, &new_layer);
			layer = &new_layer;
			delta = 1;		/* insert */
			break;
		}
	}

	value_tuple_store(layer, slot, key);
	value_tuple_store(layer, slot + 1, value);

	if (delta != 0) {
		usage = value_tuple_fetch_integer(layer, LAYER_USAGE);
		usage += delta;
		assert (usage >= 0);
		if (usage == 0) {
			/*
			 * This layer is now empty.  Unlink it,
			 * and the garbage collector will delete it.
			 */
			if (!value_is_null(prev_layer)) {
				value_tuple_store(prev_layer, LAYER_NEXT,
				    value_tuple_fetch(layer, LAYER_NEXT));
			}
		} else {
			value_tuple_store_integer(layer, LAYER_USAGE, usage);
		}
	}
}

void
value_dict_store(struct value *dict, struct value *key, struct value *value)
{
	assert(value_is_tuple(dict));
	value_store_hash(dict, key, value, value_hash);
}

unsigned int
value_dict_get_length(const struct value *dict)
{
	const struct value *layer = dict;
	unsigned int size = 0;

	while (!value_is_null(layer)) {
		size += value_tuple_fetch_integer(layer, LAYER_USAGE);
		layer = value_tuple_fetch(layer, LAYER_NEXT);
	}

	return size;
}

unsigned int
value_dict_get_layer_size(const struct value *dict)
{
	return value_tuple_get_size(dict) - LAYER_HEADER_SIZE;
}

#define DICT_ITER_LAYER 0
#define DICT_ITER_POS   1

int
value_dict_new_iter(struct value *v, struct value *dict)
{
	if (!value_tuple_new(v, &tag_iter, 2))
		return 0;

	value_tuple_store(v, DICT_ITER_LAYER, dict);
	value_tuple_store_integer(v, DICT_ITER_POS, LAYER_HEADER_SIZE);

	return 1;
}

/* returns VNULL if there are no (more) entries in the dict. */
struct value *
value_dict_iter_get_current_key(struct value *dict_iter)
{
	struct value *layer = value_tuple_fetch(dict_iter, DICT_ITER_LAYER);
	unsigned int pos = value_tuple_fetch_integer(dict_iter, DICT_ITER_POS);
	unsigned int layer_size = value_tuple_get_size(layer);
	struct value *key;

	if (pos >= layer_size) {
		layer = value_tuple_fetch(layer, LAYER_NEXT);
		if (value_is_null(layer)) {
			return &VNULL;
		}
		pos = LAYER_HEADER_SIZE;
	}

	key = value_tuple_fetch(layer, pos);
	while (value_is_null(key)) {
		pos += 2;
		if (pos >= layer_size) {
			layer = value_tuple_fetch(layer, LAYER_NEXT);
			if (value_is_null(layer)) {
				return &VNULL;
			}
			pos = LAYER_HEADER_SIZE;
		}
		key = value_tuple_fetch(layer, pos);
	}

	value_tuple_store(dict_iter, DICT_ITER_LAYER, layer);
	value_tuple_store_integer(dict_iter, DICT_ITER_POS, pos);

	return key;
}

void
value_dict_iter_advance(struct value *dict_iter)
{
	unsigned int pos = value_tuple_fetch_integer(dict_iter, DICT_ITER_POS);

	value_tuple_store_integer(dict_iter, DICT_ITER_POS, pos + 2);
}

/***** tuples as virtual machines *****/

int
value_vm_new(struct value *vm, struct value *code_tuple)
{
	if (!value_tuple_new(vm, &tag_vm, VM_SIZE))
		return 0;

	value_tuple_store(vm, VM_CODE, code_tuple);
	value_tuple_store(vm, VM_IS_DIRECT, &VFALSE);
	value_vm_reset(vm);

	return 1;
}

void
value_vm_reset(struct value *vm)
{
	value_tuple_store_integer(vm, VM_PC, 0);
	value_tuple_store(vm, VM_AR, &VNULL);
}

/*** GENERAL OPERATIONS ***/

void
value_copy(struct value *dst, const struct value *src)
{
	dst->type = src->type;
	dst->value = src->value;
}

int
value_is_null(const struct value *v)
{
	return v->type == VALUE_NULL;
}

static enum comparison
value_compare_nodups(const struct value *a, const struct value *b, struct value *seen)
{
	unsigned int i;
	struct value hidden_a, hidden_b;

	if (a->type != b->type)
		return CMP_INCOMPARABLE;

	switch (a->type) {
	case VALUE_NULL:
		return CMP_EQ;
	case VALUE_INTEGER:
		if (a->value.integer > b->value.integer) {
			return CMP_GT;
		}
		if (a->value.integer < b->value.integer) {
			return CMP_LT;
		}
		return CMP_EQ;
	case VALUE_BOOLEAN:
		if (a->value.boolean == b->value.boolean) {
			return CMP_EQ;
		}
		return CMP_INCOMPARABLE;
	case VALUE_PROCESS:
		if (a->value.process == b->value.process) {
			return CMP_EQ;
		}
		return CMP_INCOMPARABLE;
	case VALUE_LABEL:
		if (a->value.label == b->value.label) {
			return CMP_EQ;
		}
		return CMP_INCOMPARABLE;
	case VALUE_SYMBOL:
	    {
		int k = strcmp(value_symbol_get_token(a),
			       value_symbol_get_token(b));
		if (k > 0) {
			return CMP_GT;
		}
		if (k < 0) {
			return CMP_LT;
		}
		return CMP_EQ;
	    }
	case VALUE_TUPLE:
		/*
		 * First, to handle the case where tuples contain
		 * cyclic links, check to see if these are, in fact,
		 * the same tuple.
		 */
		if ((struct tuple *)(a->value.structured) ==
		    (struct tuple *)(b->value.structured))
			return CMP_EQ;

		/*
		 * XXX I would almost think that if the tags are not
		 * the same, then the tuples are incomparable...
		 */
		switch (value_compare(value_tuple_get_tag(a), value_tuple_get_tag(b))) {
			case CMP_GT:
				return CMP_GT;
			case CMP_LT:
				return CMP_LT;
			case CMP_INCOMPARABLE:
				return CMP_INCOMPARABLE;
			case CMP_EQ:
				break;
		}

		if (((struct tuple *)(a->value.structured))->size >
		    ((struct tuple *)(b->value.structured))->size) {
			return CMP_GT;
		}
		if (((struct tuple *)(a->value.structured))->size <
		    ((struct tuple *)(b->value.structured))->size) {
			return CMP_LT;
		}

		/*
		 * This more prudent check keeps in mind that we are
		 * recursively testing for equality, and stores
		 * tuples in a 'have seen' dictionary, later recursing
		 * into them only if they have not yet been seen.
		 *
		 * Note, however, that because value_dict_store() internally
		 * uses value_equal(), we can get into a different kind
		 * of inifnite cycle - mutual recursion.  To avoid that,
		 * we store encoded values based on the tuples, instead
		 * of the tuples themselves, in the 'have seen' dict.
		 */
		value_process_set(&hidden_a, (struct process *)value_get_unique_id(a));
		value_process_set(&hidden_b, (struct process *)value_get_unique_id(b));
		value_dict_store(seen, &hidden_a, &VTRUE);
		value_dict_store(seen, &hidden_b, &VTRUE);

		for (i = 0; i < ((struct tuple *)(a->value.structured))->size; i++) {
			struct value *na = value_tuple_fetch(a, i);
			struct value *nb = value_tuple_fetch(b, i);
			enum comparison c;

			if ((!value_is_null(value_dict_fetch(seen, na))) ||
			    (!value_is_null(value_dict_fetch(seen, nb))))
				return CMP_INCOMPARABLE; /* XXX ? */

			c = value_compare(na, nb);
			if (c != CMP_EQ)
				return c;
		}
		return CMP_EQ;
	}
	/* should never be reached */
	assert(a->type == VALUE_NULL);
	return 0;
}

enum comparison
value_compare(const struct value *a, const struct value *b)
{
	struct value d;
	value_dict_new(&d, 31);
	return value_compare_nodups(a, b, &d);
}

int
value_equal(const struct value *a, const struct value *b)
{
	return value_compare(a, b) == CMP_EQ;
}

/***** gc *****/

/*
 * Garbage collector.  Not a cheesy little reference counter, but
 * a real meat-and-potatoes mark-and-sweep.
 *
 * This is not particularly sophisticated; I'm more concerned with
 * correctness than performance here.
 */

static void
mark_tuple(struct value *v)
{
	struct value *k;
	unsigned int i;

	assert(value_is_tuple(v));
	for (i = 0; i < value_tuple_get_size(v); i++) {
		k = value_tuple_fetch(v, i);
		/*
		 * If the contained value is also structured,
		 * and it hasn't been marked yet, mark it too.
		 */
		if (k->type & VALUE_STRUCTURED) {
			struct structured_value *sv = k->value.structured;
			if (k->type == VALUE_TUPLE &&
			    (!(sv->admin & ADMIN_MARKED))) {
				/*
				 * It can contain other values and
				 * it hasn't been marked yet, so
				 * recursively mark its contents.
				 */
				mark_tuple(k);
			} else {
				sv->admin |= ADMIN_MARKED;
			}
		}
	}
}

/*
 * Public interface to garbage collector.
 */

void
value_gc(struct value *root)
{
	struct structured_value *sv, *sv_next, *temp_sv_head = NULL;

	/*
	 * Mark...
	 */
	mark_tuple(root);

	/*
	 * ...and sweep
	 */
	for (sv = sv_head; sv != NULL; sv = sv_next) {
		sv_next = sv->next;
		if (sv->admin & ADMIN_MARKED) {
			sv->admin &= ~ADMIN_MARKED;
			sv->next = temp_sv_head;
			temp_sv_head = sv;
		} else {
			/*
			 * Found an unreachable SV!
			 * Not much special knowledge is required to
			 * free a structured value block, so we just
			 * (un-abstractedly) inline the process here.
			 */
			free(sv);
		}
	}

	sv_head = temp_sv_head;
}
