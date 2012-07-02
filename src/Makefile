# Makefile for Kosheri.
# $Id: Makefile 146 2008-12-06 19:59:54Z catseye $

CC?=gcc
AR?=ar
RANLIB?=ranlib

O?=.o
EXE?=

OD?=./

DEBUG_PORTRAY_O?=

WARNS=	-Werror -W -Wall -Wstrict-prototypes -Wmissing-prototypes \
	-Wpointer-arith	-Wno-uninitialized -Wreturn-type -Wcast-qual \
	-Wwrite-strings -Wswitch -Wshadow -Wcast-align -Wchar-subscripts \
	-Winline -Wnested-externs -Wredundant-decls
LIBS=-L. -lruntime
CFLAGS+=-ansi -pedantic ${WARNS} ${EXTRA_CFLAGS}

RUNTIME_OBJS=	${OD}lib${O} ${OD}value${O} \
		${OD}process${O} ${OD}file${O} ${OD}stream${O} \
                ${DEBUG_PORTRAY_O} \
		${OD}render${O}
# ${OD}portray${O}

# portray and save are only needed for non-raw output
# instrtab is only needed for direct threading conversion
RUN_OBJS=	${OD}run${O} \
		${OD}load${O} \
		${OD}vm${O} ${OD}vmproc${O} \
		${OD}instrtab${O} \
		${OD}portray${O} \
                ${OD}save${O} \
		${OD}cmdline${O}

ASSEMBLE_OBJS=	${OD}assemble${O} \
		${OD}instrtab${O} \
		${OD}report${O} ${OD}scan${O} ${OD}discern${O} ${OD}chain${O} \
		${OD}gen${O} ${OD}save${O} \
		${OD}portray${O} \
		${OD}cmdline${O}

DISASM_OBJS=	${OD}disasm${O} \
		${OD}instrtab${O} \
		${OD}report${O} \
		${OD}load${O} \
		${OD}portray${O} \
		${OD}chain${O} \
		${OD}cmdline${O}

FREEZE_OBJS=	${OD}freeze${O} \
		${OD}save${O} \
		${OD}report${O} ${OD}scan${O} \
		${OD}discern${O} ${OD}chain${O} \
		${OD}cmdline${O}

THAW_OBJS=	${OD}thaw${O} \
		${OD}load${O} \
		${OD}report${O} \
                ${OD}portray${O} \
		${OD}cmdline${O}

BUILDINFO_OBJS=	${OD}buildinfo${O} \
		${OD}process${O} \
		${OD}cmdline${O}

PROGS=run${EXE} assemble${EXE} disasm${EXE} freeze${EXE} thaw${EXE} \
		buildinfo${EXE}

all: ${PROGS}

geninstr${EXE}: geninstr.o
	${CC} geninstr.o -o geninstr${EXE}

instrtab.c instrenum.h instrlab.h localtypes.h: vm.c geninstr
	./geninstr vm.c

value.h: localtypes.h

value.c: value.h

vm.h: instrenum.h instrlab.h value.h

libruntime.a: ${RUNTIME_OBJS}
	${AR} rc libruntime.a ${RUNTIME_OBJS}
	${RANLIB} libruntime.a

run.c: vm.h

run${EXE}: ${RUN_OBJS} libruntime.a
	${CC} ${RUN_OBJS} ${LIBS} -o run${EXE}
assemble${EXE}: ${ASSEMBLE_OBJS} libruntime.a
	${CC} ${ASSEMBLE_OBJS} ${LIBS} -o assemble${EXE}
disasm${EXE}: ${DISASM_OBJS} libruntime.a
	${CC} ${DISASM_OBJS} ${LIBS} -o disasm${EXE}
freeze${EXE}: ${FREEZE_OBJS} libruntime.a
	${CC} ${FREEZE_OBJS} ${LIBS} -o freeze${EXE}
thaw${EXE}: ${THAW_OBJS} libruntime.a
	${CC} ${THAW_OBJS} ${LIBS} -o thaw${EXE}

buildinfo${EXE}: ${BUILDINFO_OBJS} libruntime.a
	${CC} ${BUILDINFO_OBJS} ${LIBS} -o buildinfo${EXE}


# when DEBUG is defined, save.o, load.o, and parse.o depend on portray.o
debug: clean
	${MAKE} DEBUG_PORTRAY_O="${OD}portray${O}" \
                EXTRA_CFLAGS="-g -DDEBUG"

profiled: clean
	${MAKE} EXTRA_CFLAGS="-pg"

tool: clean
	${MAKE} EXTRA_CFLAGS="-DNDEBUG -Os" LIBS="-L. -lruntime -s"

#  -finline-functions ?

static: clean
	${MAKE} EXTRA_CFLAGS="-DNDEBUG -Os" LIBS="-L. -lruntime -s -static"

standalone: clean
	${MAKE} EXTRA_CFLAGS="-DNDEBUG -DSTANDALONE -DCTYPE_IS_BUILTIN -Os -static -nostdlib" LIBS="-L. -lruntime -nostdlib -s"

win: clean
	${MAKE} EXE=.exe EXTRA_CFLAGS="-DNDEBUG -mno-cygwin" all

wintool: clean
	${MAKE} EXE=.exe EXTRA_CFLAGS="-DNDEBUG -Os -static -mno-cygwin" LIBS="-L. -lruntime -s"

clean:
	rm -rf ${OD}*${O} *.so *.a *.core *.vm *.exe instrtab.c instrenum.h instrlab.h geninstr *.stackdump ${PROGS} foo.* LISTING OUTPUT
