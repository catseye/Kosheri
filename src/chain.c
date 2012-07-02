/*
 * chain.c
 * Routines for collecting chains.
 * $Id: chain.c 143 2008-07-18 06:42:22Z catseye $
 */

#include "lib.h"
#include "chain.h"
#include "value.h"

struct chain {
	struct chain	*next;
	struct value	 value;
};

struct chain *
add_to_chain(struct chain *c, struct value *v)
{
	struct chain *n;

	n = malloc(sizeof(struct chain));
	n->next = NULL;
	value_copy(&n->value, v);
	if (c != NULL) {
		c->next = n;
	}
	return n;
}

void
populate_tuple_from_chain(struct value *v, struct chain *c)
{
	unsigned int i = 0;

	while (c != NULL) {
		value_tuple_store(v, i, &c->value);
		i++;
		c = c->next;
	}
}

struct value *
search_chain(struct chain *c, struct value *f)
{
	while (c != NULL) {
		if (value_equal(f, &c->value))
			return &c->value;
		c = c->next;
	}
	return NULL;
}

void
free_chain(struct chain *c)
{
	struct chain *next;

	while (c != NULL) {
		next = c->next;
		free(c);
		c = next;
	}
}
