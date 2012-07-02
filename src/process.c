/*
 * process.c
 * Cooperative processes.
 */


#include "lib.h"

#include "process.h"

struct process *
process_new(void)
{
	struct process *p;

	p = malloc(sizeof(struct process));
	p->head = NULL;
	p->tail = NULL;
	p->run = NULL;
	p->aux = NULL;
	p->waiting = 0;
	p->done = 0;
	p->next = NULL;

	return p;
}

void
process_enqueue(struct process *p, const struct value *v)
{
	struct message *m;

	m = malloc(sizeof(struct message));
	value_copy(&m->value, v);
	m->prev = NULL;
	m->next = p->head;
	if (p->head != NULL)
		p->head->prev = m;
	p->head = m;
	if (p->tail == NULL)
		p->tail = m;
}

int
process_dequeue(struct process *p, struct value *v)
{
	struct message *m;

	m = p->tail;
	if (m == NULL)
		return 0;
	value_copy(v, &m->value);

	/* remove message from queue */
	assert(m->next == NULL);
	if (m->prev == NULL)
		p->head = NULL;
	else
		m->prev->next = NULL;
	p->tail = m->prev;

	free(m);

	return 1;
}

void
process_run(struct process *p)
{
	assert(p->run != NULL);
	p->run(p);
}

void
process_free(struct process *p)
{
	struct message *m, *n;

	/* assert(p->done); */
	m = p->head;
	while (m != NULL) {
		n = m->next;
		free(m);
		m = n;
	}
	free(p);
}
