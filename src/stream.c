/*
 * stream.c
 * Routines for communicating with stream-like processes.
 */

#include "lib.h"

#include "process.h"

#include "stream.h"

static void
read_receiver_run(struct process *self)
{
	struct value msg;
	const char *token;
	int length;
	int i;

	assert(self != NULL);
	assert(self->aux != NULL);

	while (process_dequeue(self, &msg)) {
		token = value_symbol_get_token(&msg);
		length = value_symbol_get_length(&msg);
		for (i = 0; i < length; i++) {
			((char *)self->aux)[i] = token[i];
		}
		/* XXX assert the queue is empty? */
	}
}

static struct process *
read_receiver_new(void *buffer)
{
	struct process *p;

	p = process_new();
	p->run = read_receiver_run;
	p->aux = buffer;

	return p;
}

static void
eof_receiver_run(struct process *self)
{
	struct value msg;

	assert(self != NULL);
	assert(self->aux != NULL);

	while (process_dequeue(self, &msg)) {
		*(int *)(self->aux) = value_get_boolean(&msg);
		/* XXX assert the queue is empty? */
	}
}

static struct process *
eof_receiver_new(int *result)
{
	struct process *p;

	p = process_new();
	p->run = eof_receiver_run;
	p->aux = result;

	return p;
}

void
stream_write(struct process *self, struct process *p, const void *data, unsigned int size)
{
	struct value msg, tag;

	self = self;

	value_symbol_new(&tag, "write", 5);
	value_tuple_new(&msg, &tag, 1);
	value_symbol_new(value_tuple_fetch(&msg, 0), data, size);
	process_enqueue(p, &msg);
	process_run(p);
}

void
stream_read(struct process *self, struct process *p, void *buffer, unsigned int size)
{
	struct value msg, tag;
	struct process *receiver;

	if (self == NULL) {
		receiver = read_receiver_new(buffer);
	} else {
		receiver = self;
	}

	value_symbol_new(&tag, "read", 4);
	value_tuple_new(&msg, &tag, 2);
	value_process_set(value_tuple_fetch(&msg, 0), receiver);
	value_integer_set(value_tuple_fetch(&msg, 1), size);
	process_enqueue(p, &msg);
	process_run(p);

	if (receiver == self) {
		return;
	}

	process_run(receiver);
	process_free(receiver);
}

int
stream_is_at_end(struct process *self, struct process *p)
{
	struct value msg, tag;
	struct process *receiver;
	int result;

	if (self == NULL) {
		receiver = eof_receiver_new(&result);
	} else {
		receiver = self;
	}

	value_symbol_new(&tag, "eof", 3);
	value_tuple_new(&msg, &tag, 1);
	value_process_set(value_tuple_fetch(&msg, 0), receiver);
	process_enqueue(p, &msg);
	process_run(p);

	if (receiver == self) {
		return 0;
	}

	process_run(receiver);
	process_free(receiver);

	return result;
}

void
stream_close(struct process *self, struct process *p)
{
	struct value msg, tag;

	self = self;

	value_symbol_new(&tag, "close", 5);
	value_tuple_new(&msg, &tag, 0);
	process_enqueue(p, &msg);
	process_run(p);
}
