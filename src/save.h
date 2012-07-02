/*
 * save.h
 * Structures and prototypes for saving values to processes.
 */

#ifndef __SAVE_H_
#define __SAVE_H_

struct process;
struct value;

int		 value_save(struct process *, struct value *);

#endif /* !__SAVE_H_ */
