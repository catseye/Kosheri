/*
 * vm.h
 * Virtual machine structures and prototypes.
 * $Id: vm.h 100 2006-02-25 02:20:09Z catseye $
 */

#ifndef __VM_H_
#define __VM_H_

#include "value.h"

struct process;

void		 vm_run(struct value *, struct process *, unsigned int);

#endif /* !__VM_H_ */
