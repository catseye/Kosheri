/*
 * discern.h
 * Prototypes and structures for parsing values (only; not full
 * program source code) from textual streams.
 */

#ifndef __DISCERN_H_
#define __DISCERN_H_

struct value;
struct scanner;

int value_discern(struct value *, struct scanner *);

#endif /* !__DISCERN_H_ */
