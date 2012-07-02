/*
 * value.h
 * Values.
 */

#ifndef __VALUE_H_
#define __VALUE_H_

#include "localtypes.h"

/*
 * Types of values.
 */
#define	VALUE_STRUCTURED 8
enum value_type {
	VALUE_NULL	= 0,
	VALUE_INTEGER	= 1,
	VALUE_BOOLEAN	= 2,
	VALUE_PROCESS	= 3,
	VALUE_LABEL	= 4,

	VALUE_SYMBOL	= (VALUE_STRUCTURED | 1),
	VALUE_TUPLE	= (VALUE_STRUCTURED | 2)
};

#ifdef DEBUG
extern const char *type_name_table[];
#endif

typedef void * clabel;

struct process;

/*
 * Simple values.
 * These exist directly on the stack, and are not garbage-collected.
 */
struct value {
	enum value_type		 type;		/* VALUE_ */
	union {
		int			 integer;
		int			 boolean;
		struct process		*process;
		clabel			 label;
		struct structured_value	*structured;
	} value;
};

extern struct value VNULL;
extern struct value VFALSE;
extern struct value VTRUE;

extern struct value tag_ar;
extern struct value tag_dict;
extern struct value tag_list;
extern struct value tag_vm;

/*
 * Describe how two values compare.
 */
enum comparison {
	CMP_EQ,
	CMP_LT,
	CMP_GT,
	CMP_INCOMPARABLE
};

/* Prototypes */

/*
 * 'struct value *' parameters may NEVER be NULL.
 *
 * Only functions whose names end in _new allocate a struct value.
 */

/*
 * General functions.
 */
void		 value_copy(struct value *, const struct value *);

int		 value_is_null(const struct value *);
int		 value_is_integer(const struct value *);
int		 value_is_tuple(const struct value *);

int		 value_equal(const struct value *, const struct value *);
enum comparison	 value_compare(const struct value *, const struct value *);

		 /* public interface to garbage collector */
void		 value_gc(struct value *);

/*
 * Unstructured values.
 */

void		 value_integer_set(struct value *, int);
void		 value_boolean_set(struct value *, int);
void		 value_process_set(struct value *, struct process *);
void		 value_label_set(struct value *, clabel);

int		 value_get_integer(const struct value *);
int		 value_get_boolean(const struct value *);
clabel		 value_get_label(const struct value *);
struct process	*value_get_process(const struct value *);

/*
 * Structured values.
 */

/*
 * Retrieve an integer code that uniquely identifies this value.
 * Intended for debug output only.
 * Precondition: value is a structured value.
 */
PTR_INT		 value_get_unique_id(const struct value *);

/*
 * Symbols.
 */

/*
 * Allocate a new symbol value, with the given character string
 * (with the given length) as its token, and set the given value to it.
 * Returns true upon success, false if memory could not be allocated.
 * Precondition: value is not null, and the token is not null.
 */
int              value_symbol_new(struct value *, const char *, unsigned int);

/*
 * Allocate a new symbol value with the given length and set the
 * given value to it.  Return a pointer to the start of the character
 * data for the symbol, for later population by the caller.
 * Returns null if memory could not be allocated.
 * Precondition: value is not null, and the token is not null.
 */
char		*value_symbol_new_buffer(struct value *, unsigned int);
const char	*value_symbol_get_token(const struct value *);
unsigned int	 value_symbol_get_length(const struct value *);

/*
 * Tuples.
 */

int		 value_tuple_new(struct value *, struct value *, unsigned int);
struct value	*value_tuple_get_tag(const struct value *);
unsigned int	 value_tuple_get_size(const struct value *);
struct value	*value_tuple_fetch(const struct value *, unsigned int);
void		 value_tuple_store(struct value *, unsigned int, const struct value *);
int		 value_tuple_fetch_integer(const struct value *, unsigned int);
void		 value_tuple_store_integer(struct value *, unsigned int, int);
clabel		 value_tuple_fetch_label(const struct value *, unsigned int);

/*
 * Dictionaries.
 * Dictionaries are represented by a linked list of "layers", where
 * each layer is a tuple of the given size plus some header slots.
 */

int		 value_dict_new(struct value *, unsigned int);
struct value	*value_dict_fetch(const struct value *, const struct value *);
void		 value_dict_store(struct value *, struct value *, struct value *);
unsigned int	 value_dict_get_length(const struct value *);
unsigned int	 value_dict_get_layer_size(const struct value *);

/*
 * Dictionary iterators.
 */

int              value_dict_new_iter(struct value *, struct value *);
struct value    *value_dict_iter_get_current_key(struct value *);
void             value_dict_iter_advance(struct value *);

/*
 * Activation records.
 * Activation records are represented by tuple with 4 header entries.
 * The remainder of the entries are local variables stored in the AR.
 */

#define AR_CALLER	0	/* ar: the ar that called us */
#define AR_ENCLOSING	1	/* ar: the ar lexically outside us */
#define AR_PC		2	/* integer: code pos when resumed */
#define AR_TOP		3	/* integer: current top of stack (init. 4) */

#define AR_HEADER_SIZE	4

int		 value_ar_new(struct value *, unsigned int, struct value *, struct value *, unsigned int);
struct value	*value_ar_pop(struct value *);
void		 value_ar_push(struct value *, struct value *);
void		 value_ar_xfer(struct value *, struct value *, int);

/*
 * Virtual machines.
 * A virtual machine is represented by a tuple with 4 entries:
 * - the first is an integer offset: the program counter
 * - the second is a tuple representing the current activation record
 * - the third is a boolean: is this code direct-threaded or not?
 * - the fourth is a tuple containing the code as VM instructions
 */

#define VM_PC		0
#define VM_AR		1
#define VM_IS_DIRECT	2
#define VM_CODE		3

#define VM_SIZE		4

int		 value_vm_new(struct value *, struct value *);
void		 value_vm_reset(struct value *);

#endif /* !__VALUE_H_ */

