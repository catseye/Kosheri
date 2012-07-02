/*
 * discern.c
 * Routines for parsing values (only; not program source) from a file-like process.
 * $Id: discern.c 143 2008-07-18 06:42:22Z catseye $
 */

#include "lib.h"

#include "scan.h"
#include "value.h"
#include "discern.h"
#include "chain.h"

/*
 * A parser for values, inspired somewhat by Scheme/LISP S-expressions
 * and Prolog/Erlang terms.
 */
int
value_discern(struct value *top, struct scanner *sc)
{
	if (scanner_tokeq(sc, "<")) {
		struct chain *front, *back;
		struct value tag, inner;
		unsigned int size = 1;

		/* Tuple. */
		scanner_scan(sc);
		value_discern(&tag, sc);
		scanner_expect(sc, ":");
		value_discern(&inner, sc);
		back = front = add_to_chain(NULL, &inner);
		while (scanner_tokeq(sc, ",")) {
			scanner_scan(sc);
			value_discern(&inner, sc);
			back = add_to_chain(back, &inner);
			size++;
		}
		scanner_expect(sc, ">");
		value_tuple_new(top, &tag, size);
		populate_tuple_from_chain(top, front);
		free_chain(front);
		return 1;
	} else if (scanner_tokeq(sc, "[")) {
                struct value inner, *left, right;

		/* List: Sequence of tail-nested Pairs. */
		scanner_scan(sc);
		if (scanner_tokeq(sc, "]")) {
			scanner_scan(sc);
			value_copy(top, &VNULL);
			return 1;
		}
		value_tuple_new(top, &tag_list, 2);
		value_discern(&inner, sc);
		value_tuple_store(top, 0, &inner);
		/*value_tuple_store(top, 1, VNULL);*/
		left = top;
		while (scanner_tokeq(sc, ",")) {
			scanner_scan(sc);
			value_tuple_new(&right, &tag_list, 2);
			value_discern(&inner, sc);
			value_tuple_store(&right, 0, &inner);
			/*value_set_index(right, 1, VNULL);*/
			value_tuple_store(left, 1, &right);
			left = value_tuple_fetch(left, 1);
		}
		if (scanner_tokeq(sc, "|")) {
			scanner_scan(sc);
			value_discern(&right, sc);
			value_tuple_store(left, 1, &right);
		}
		scanner_expect(sc, "]");
		return 1;
	} else if (scanner_tokeq(sc, "{")) {
		struct value left, right;

		/* Dictionary: associations between keys and values. */
		scanner_scan(sc);
		value_dict_new(top, 31);

		value_discern(&left, sc);
		scanner_expect(sc, "=");
		value_discern(&right, sc);
		value_dict_store(top, &left, &right);
		while (scanner_tokeq(sc, ",")) {
			scanner_scan(sc);
			value_discern(&left, sc);
			scanner_expect(sc, "=");
			value_discern(&right, sc);
			value_dict_store(top, &left, &right);
		}
		scanner_expect(sc, "}");
		return 1;
	} else if (k_isdigit(scanner_token_string(sc)[0])) {
		/* Integer. */
		value_integer_set(top, k_atoi(scanner_token_string(sc),
					      scanner_token_length(sc)));
		scanner_scan(sc);
		return 1;
	} else {
		/* Symbol. */
		value_symbol_new(top, scanner_token_string(sc),
				 scanner_token_length(sc));
		scanner_scan(sc);
		return 1;
	}
}
