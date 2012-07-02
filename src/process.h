/*
 * process.h
 * Cooperative concurrent processes.
 */

#ifndef __PROCESS_H_
#define __PROCESS_H_

#include "value.h"

struct process;

typedef void (*runfunc)(struct process *);

struct process {
	int		 waiting;
	int		 done;
	runfunc		 run;
	void		*aux;
	struct value	 aux_value;
	struct message	*head;
	struct message	*tail;
	struct process	*next;
};

struct message {
	struct message	*next;
	struct message	*prev;
	struct value	 value;
};

/* Prototypes */

struct process	*process_new(void);

/*
 * places the value in the process's mailbox.  This is what the VM instruction SEND
 *  does.  SEND also checks if the destination process is currently waiting, (and if
 *  its mailbox is empty?), and if it is, it wakes it up so that it can deal immediately
 *  with the message.  Maybe.  I mean, this behaviour sounds good, but maybe we shouldn't
 *  guarantee it.
 *
 * We should also prevent side-effecting of that value.  Maybe values can be "owned"...
 */
void		 process_enqueue(struct process *, const struct value *);

/*
 * retrieves a value from the process's mailbox.  p should be self.  This is what
 * the RECV VM instruction does.  If there is no message, the process should set its
 * waiting flag to true, and yield.
 */
int		 process_dequeue(struct process *, struct value *);

/*
 * Let the process p execute for a bit.  Concurrency is cooperative here, so p promises
 * that it will return from this function "in a little while".  For the VM, this is not
 * hard; we return after executing 100 or so instructions.  For native code, it should
 * be carefully written!
 */
void		 process_run(struct process *);

void		 process_free(struct process *);

#endif /* !__PROCESS_H_ */

