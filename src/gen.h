/*
 * gen.h
 * Accumulate values in tuples with extents.
 * $Id: gen.h 144 2008-07-19 01:57:04Z catseye $
 */

#ifndef __GEN_H_
#define __GEN_H_

#include "value.h"

int			 gen_new(struct value *, struct value *, unsigned int);
int			 gen_new_default(struct value *);
void			 gen_value(struct value *, struct value *);
void			 gen_integer(struct value *, int);
int			 gen_define_label(struct value *, struct value *);
void			 gen_gen_label_ref(struct value *, struct value *);
void			 gen_flatten(struct value *, struct value *);

#endif /* !__GEN_H_ */
