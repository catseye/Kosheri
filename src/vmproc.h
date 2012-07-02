/*
 * vmproc.h
 * Virtual machine code backed processes.
 */

#ifndef __VMPROC_H_
#define __VMPROC_H_

struct value;
struct process;

struct process	*vmproc_new(struct value *);

#endif /* !__VMPROC_H_ */

