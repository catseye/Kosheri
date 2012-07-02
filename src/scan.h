/*
 * scan.h
 * Lexical scanner structures and prototypes.
 * $Id: scan.h 139 2008-07-16 09:56:31Z catseye $
 */

#ifndef __SCAN_H_
#define __SCAN_H_

#include "report.h"

struct scanner;

struct scanner *scanner_new(struct reporter *);
void		scanner_free(struct scanner *);

void		scanner_reset(struct scanner *);
int		scanner_attach(struct scanner *, struct process *, const char *);
int		scanner_open(struct scanner *, const char *);
void		scanner_close(struct scanner *);

void 		scanner_scan(struct scanner *);
void		scanner_expect(struct scanner *, const char *);
void		scanner_scanline(struct scanner *);

int             scanner_tokeq(struct scanner *, const char *);
const char     *scanner_token_string(struct scanner *);
int             scanner_token_length(struct scanner *);

int             scanner_eof(struct scanner *);
const char     *scanner_filename(struct scanner *);
int             scanner_line(struct scanner *);
int             scanner_column(struct scanner *);

void            scanner_report(struct scanner *, enum report_type, const char *, ...);

#endif /* !__SCAN_H_ */
