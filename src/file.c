/*
 * file.c
 * Native processes for communicating with (reading, writing) files,
 * exposing a stream-like interface.
 */

#include <stdio.h>

#include "lib.h"
#include "process.h"
#include "file.h"

#ifdef DEBUG
#include "stream.h"
#include "portray.h"
#include "cmdline.h"
#endif

static void run(struct process *p)
{
	struct value msg;
	struct value response;
	struct process *sender;
	size_t result;
	char *buffer;
	size_t size;

	assert(p != NULL);
	assert(p->aux != NULL);

	while (process_dequeue(p, &msg)) {
#ifdef DEBUG
/*
		stream_write(NULL, process_err, "-->", 3);
		value_portray(process_err, &msg);
		stream_write(NULL, process_err, "<--", 3);
*/
#endif
		if (value_is_tuple(&msg)) {
			const char *tag = value_symbol_get_token(value_tuple_get_tag(&msg));
		        if (strcmp(tag, "write") == 0) {
				struct value *payload = value_tuple_fetch(&msg, 0);

				result = fwrite(
				    value_symbol_get_token(payload),
				    value_symbol_get_length(payload),
				    1, (FILE *)p->aux
				);
                        	if (result) {
					/* some kind of error occurred */
				}
		        } else if (strcmp(tag, "read") == 0) {
				sender = value_get_process(value_tuple_fetch(&msg, 0));
				size = value_get_integer(value_tuple_fetch(&msg, 1));

				buffer = value_symbol_new_buffer(&response, size);
				result = fread(buffer, size, 1, (FILE *)p->aux);
                        	if (result) {
					/* some kind of error occurred, or we just need more */
				}
				process_enqueue(sender, &response);
			} else if (strcmp(tag, "eof") == 0) {
				sender = value_get_process(value_tuple_fetch(&msg, 0));

				value_boolean_set(&response, feof((FILE *)p->aux));
				process_enqueue(sender, &response);
			} else if (strcmp(tag, "close") == 0) {
				fclose((FILE *)p->aux);
				p->aux = NULL;
			}
		}
	}

	p->waiting = 1;
}

static struct process *
file_fopen(FILE *file)
{
	struct process *p;

	p = process_new();
	p->run = run;
	p->aux = file;

	return p;
}

struct process *
file_open(const char *locator, const char *mode)
{
	FILE *f;

	if (strcmp(locator, "*stdin") == 0) {
		return file_fopen(stdin);
	}
	if (strcmp(locator, "*stdout") == 0) {
		return file_fopen(stdout);
	}
	if (strcmp(locator, "*stderr") == 0) {
		return file_fopen(stderr);
	}

	if ((f = fopen(locator, mode)) == NULL) {
		/* XXX only if mode contains a 'strict' char flag... */
		/* XXX this should use report.c somehow someday */
		fprintf(stderr,
		   "Could not open '%s' in mode '%s'", locator, mode);
		exit(1);
	}

	return file_fopen(f);
}
