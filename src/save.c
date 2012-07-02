/*
 * save.c
 * Save values to a stream-like process.
 */

#include "lib.h"
#include "stream.h"

#include "save.h"
#include "value.h"

#ifdef DEBUG
#include "cmdline.h"
#include "portray.h"
#include "render.h"
#endif

int
value_save(struct process *p, struct value *value)
{
	unsigned char squeeze = (unsigned char)value->type;

#ifdef DEBUG
	process_render(process_err, "(save:%s ", type_name_table[(int)squeeze]);
        if (value->type == VALUE_SYMBOL) {
            process_render(process_err, "[%d] ", value_symbol_get_length(value));
        }
	value_portray(process_err, value);
	process_render(process_err, ")\n");
#endif

	stream_write(NULL, p, &squeeze, sizeof(squeeze));

	if ((value->type & VALUE_STRUCTURED) == 0) {
		stream_write(NULL, p, &value->value, sizeof(value->value));
	} else {
		unsigned int length, i;

		switch (value->type) {
		case VALUE_SYMBOL:
		    {
			const char *token = value_symbol_get_token(value);
			length = value_symbol_get_length(value);
			stream_write(NULL, p, &length, sizeof(length));
			stream_write(NULL, p, token, sizeof(char) * length);
			break;
		    }
		case VALUE_TUPLE:
		    {
                        struct value *tag = value_tuple_get_tag(value);
			value_save(p, tag);
                        /* XXX should eventually dispatch to a handler based on tag. */
                        if (value_equal(tag, &tag_dict)) {
                          	struct value dict_iter;
                                struct value *key;

                                value_dict_new_iter(&dict_iter, value);

                                length = value_dict_get_layer_size(value);
                                stream_write(NULL, p, &length, sizeof(length));
                                length = value_dict_get_length(value);
                                stream_write(NULL, p, &length, sizeof(length));
                          
                                key = value_dict_iter_get_current_key(&dict_iter);
                                while (!value_is_null(key)) {
                                        value_save(p, key);
                                        value_save(p, value_dict_fetch(value, key)); /* XXX not so good; use iter */
                                        value_dict_iter_advance(&dict_iter);
                                        key = value_dict_iter_get_current_key(&dict_iter);
                                }
                        } else {
                                length = value_tuple_get_size(value);
                                stream_write(NULL, p, &length, sizeof(length));
                                for (i = 0; i < length; i++) {
                                        value_save(p, value_tuple_fetch(value, i));
                                }
                        }
			break;
		    }
		default:
			assert(value->type == VALUE_SYMBOL ||
			       value->type == VALUE_TUPLE);
			break;
		}
	}

	return 1;
}
