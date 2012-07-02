/*
 * load.h
 * Structures and prototypes for loading values from file-like processes.
 * $Id: load.h 107 2006-03-16 23:55:56Z catseye $
 */

#ifndef __LOAD_H_
#define __LOAD_H_

struct process;
struct value;

int value_load(struct value *, struct process *);

#endif /* !__LOAD_H_ */
