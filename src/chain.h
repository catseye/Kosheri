/*
 * chain.h
 * Prototypes for chains.
 */

#ifndef __CHAIN_H_
#define __CHAIN_H_

#include "value.h"

struct chain;

struct chain	*add_to_chain(struct chain *, struct value *);
void		 populate_tuple_from_chain(struct value *, struct chain *);
struct value	*search_chain(struct chain *, struct value *);
void		 free_chain(struct chain *);

#endif /* !__CHAIN_H_ */
