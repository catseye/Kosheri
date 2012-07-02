/*
 * load.c
 * Load values from a stream-like process.
 */

#include "lib.h"

#include "load.h"
#include "value.h"
#include "stream.h"

#ifdef DEBUG
#include "cmdline.h"
#include "portray.h"
#include "render.h"
#endif

int
value_load(struct value *value, struct process *p)
{
	unsigned int length, i;
	char *buffer;
	unsigned char squeeze;

	stream_read(NULL, p, &squeeze, sizeof(squeeze));
	value->type = (enum value_type)squeeze;

#ifdef DEBUG
	process_render(process_err, "(load:%s ", type_name_table[value->type]);
#endif

	if ((value->type & VALUE_STRUCTURED) == 0) {
		stream_read(NULL, p, &value->value, sizeof(value->value));
	} else {
		switch (value->type) {
		case VALUE_SYMBOL:
		    {
			stream_read(NULL, p, &length, sizeof(length));
			buffer = malloc(length);
			stream_read(NULL, p, buffer, sizeof(char) * length);
			if (!value_symbol_new(value, buffer, length)) {
                                free(buffer);
                                return 0;
                        }
			free(buffer);
			break;
		    }
		case VALUE_TUPLE:
		    {
			struct value tag;
                        if (!value_load(&tag, p))
                                return 0;
                        /* XXX should eventually dispatch to a handler based on tag? */
                        if (value_equal(&tag, &tag_dict)) {
                                struct value key, val;

                                stream_read(NULL, p, &length, sizeof(length)); /* length is layer_size here */
                                value_dict_new(value, length);  /* xxx */
                                stream_read(NULL, p, &length, sizeof(length)); /* length is num entries here */
                                for (i = 0; i < length; i++) {
                                        value_load(&key, p);
                                        value_load(&val, p);
                                        value_dict_store(value, &key, &val);
                                }
                        } else {
                                struct value val;

                                stream_read(NULL, p, &length, sizeof(length));
                                value_tuple_new(value, &tag, length);
                                for (i = 0; i < length; i++) {
                                        value_load(&val, p);
                                        value_tuple_store(value, i, &val);
                                }
                        }
			break;
		    }
		default:
			assert(value->type == VALUE_SYMBOL ||
			       value->type == VALUE_TUPLE);
			return 0;
		}
	}

#ifdef DEBUG
	value_portray(process_err, value);
	process_render(process_err, ")\n");
#endif
	return 1;
}
