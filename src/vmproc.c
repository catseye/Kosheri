/*
 * vmproc.c
 * Virtual machine code backed processes.
 */

#include "lib.h"

#include "vmproc.h"

#include "process.h"
#include "vm.h"

static void
run(struct process *p)
{
	vm_run(&p->aux_value, p, 100);
}

struct process *
vmproc_new(struct value *vm)
{
	struct process *p;

	p = process_new();
	p->run = run;
	value_copy(&p->aux_value, vm);
	p->waiting = 0;

	return p;
}
