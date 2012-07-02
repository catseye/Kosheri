/*
 * assemble.c
 * Assembler for virtual machine.
 * $Id: assemble.c 146 2008-12-06 19:59:54Z catseye $
 */

#include "lib.h"
#include "cmdline.h"
#include "value.h"

#include "stream.h"
#include "file.h"

#include "scan.h"
#include "discern.h"

#include "report.h"

#include "gen.h"
#include "instrtab.h"
#include "save.h"

static struct value labels;

/* Routines */

static struct value *
fetch_label(const char *token, unsigned int length)
{
        struct value lab_text;

        value_symbol_new(&lab_text, token, length);
	return value_dict_fetch(&labels, &lab_text);
}

static void
store_label(const char *token, unsigned int length, struct value *label)
{
        struct value lab_text;

        value_symbol_new(&lab_text, token, length);
	value_dict_store(&labels, &lab_text, label);
}

static void
assemble(struct scanner *sc, struct value *gen)
{
	struct opcode_entry *oe;
	struct value label;
	int count;

	while (!scanner_eof(sc)) {
		if (scanner_tokeq(sc, ";")) {	/* A comment - ignore rest of line. */
			scanner_scanline(sc);
			continue;
		}
		if (scanner_tokeq(sc, ":")) {	/* A label - associate string with addr. */
                        const char *str;
                        int len;

			scanner_scan(sc);
                        str = scanner_token_string(sc);
                        len = scanner_token_length(sc);
			value_copy(&label, fetch_label(str, len));
			/* This may cause backpatching */
			if (gen_define_label(gen, &label)) {
				store_label(str, len, &label);
			} else {
				scanner_report(sc, REPORT_ERROR, "Label already defined");
			}
			scanner_scan(sc);
			continue;
		}

		for (oe = opcode_table; oe->token != NULL; oe++) {
			if (scanner_tokeq(sc, oe->token))
				break;
		}
		if (oe->token == NULL) {
			scanner_report(sc, REPORT_ERROR, "Unrecognized token");
			scanner_scan(sc);
			continue;
		}

		gen_integer(gen, oe->opcode);
		scanner_scan(sc);

		for (count = 0; count < oe->arity; count++) {
			if (scanner_tokeq(sc, ":")) {
                                const char *str;
                                int len;

				scanner_scan(sc);           
                                str = scanner_token_string(sc);
                                len = scanner_token_length(sc);
				value_copy(&label, fetch_label(str, len));
				gen_gen_label_ref(gen, &label);
				store_label(str, len, &label);
				scanner_scan(sc);
				continue;
			} else if (scanner_tokeq(sc, "#")) {
				struct value v;

				scanner_scan(sc);
				value_discern(&v, sc);
				gen_value(gen, &v);
				continue;
			} else {
				scanner_report(sc, REPORT_WARNING,
				    "Unrecognized argument");
				scanner_scan(sc);
			}
		}
	}
}

static void
assemble_main(struct value *args, struct value *result)
{
	struct process *out;
	struct scanner *sc;
        struct reporter *r;
	struct value gen, flat; /* the generator that we will use to build vm code */
	struct value *asmfile, *vmfile;
        struct value asmfile_sym, vmfile_sym;

  	r = reporter_new("Assembly", NULL, 1);

        value_symbol_new(&asmfile_sym, "asmfile", 7);
        value_symbol_new(&vmfile_sym, "vmfile", 6);

        assert(value_is_tuple(args));
  	asmfile = value_dict_fetch(args, &asmfile_sym);
	vmfile = value_dict_fetch(args, &vmfile_sym);

	/*
	 * Generate.
	 */
	sc = scanner_new(r);
  
	if (!scanner_open(sc, value_symbol_get_token(asmfile))) {
            value_integer_set(result, 1);
            return;
        }
        
	gen_new_default(&gen);
	value_dict_new(&labels, 8);
	assemble(sc, &gen);
	gen_integer(&gen, INSTR_EOF);
	scanner_close(sc);
	scanner_free(sc);

	/*
	 * Write out.
	 */
	out = file_open(value_symbol_get_token(vmfile), "w");
        gen_flatten(&gen, &flat);
	value_save(out, &flat);
	stream_close(NULL, out);

	value_integer_set(result, reporter_has_errors(r) ? 1 : 0);
        reporter_free(r);
}

MAIN(assemble_main)
