/*
 * run.c
 * Main program segment of Kosheri virtual machine.
 */

#include "lib.h"
#include "file.h"
#include "stream.h"
#include "cmdline.h"

#include "process.h"
#include "vmproc.h"
#include "load.h"

#include "value.h"

#ifdef DEBUG
#include "render.h"
#endif

static void
run_main(struct value *args, struct value *result)
{
	struct value vm;	/* virtual machine we will run */
        struct value vmfile_sym;

        struct value code;      /* code for the virtual machine */
	struct process *in;	/* file process we will load it from */
	struct process *first;	/* main process the VM will run in */
	struct process *curr;	/* current process in our schedule */
	struct process *next;
	struct value *vmfile;
  
        value_symbol_new(&vmfile_sym, "vmfile", 6);
	vmfile = value_dict_fetch(args, &vmfile_sym);

	in = file_open(value_symbol_get_token(vmfile), "r");
	value_load(&code, in);
	stream_close(NULL, in);

        value_vm_new(&vm, &code);
	curr = first = vmproc_new(&vm);
	while (first != NULL) {
#ifdef DEBUG
		process_render(process_err, "Running process %d\n", curr);
#endif
		process_run(curr);
		next = curr->next;
		while (next != NULL && next->done) {
			curr->next = next->next;
			process_free(next);
			next = curr->next;
		}
		curr = next;
		if (curr == NULL) {
			while (first != NULL && first->done) {
				next = first->next;
				process_free(first);
				first = next;
			}
			curr = first;
		}
	}
  
        value_integer_set(result, 0);
}

MAIN(run_main)
