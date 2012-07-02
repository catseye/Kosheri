/*
 * instrtab.h
 * Table of VM instructions, mapping names to opcodes.
 * $Id: instrtab.h 95 2006-02-14 01:33:18Z catseye $
 */

#ifndef __INSTRTAB_H_
#define __INSTRTAB_H_

#include "instrenum.h"

enum optype {
	OPTYPE_NONE,
	OPTYPE_INT,
	OPTYPE_ADDR,
	OPTYPE_VALUE
};

struct opcode_entry {
	const char	*token;
	enum opcode	 opcode;
	int		 arity;
	enum optype	 optype;
};

extern struct opcode_entry opcode_table[];

#endif /* !__INSTRTAB_H_ */
