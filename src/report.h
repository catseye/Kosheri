/*
 * report.h
 * Error/warning reporter.
 * $Id: report.h 139 2008-07-16 09:56:31Z catseye $
 */

#ifndef __REPORT_H_
#define __REPORT_H_

#include <stdarg.h>

struct process;

enum report_type {
	REPORT_WARNING,
	REPORT_ERROR
};

struct reporter;

struct reporter *reporter_new(const char *, struct process *, int);
int		 reporter_finish(struct reporter *);
void		 reporter_free(struct reporter *);

struct process	*reporter_stream(struct reporter *);
int		 reporter_has_errors(struct reporter *);

void		 report(struct reporter *, enum report_type, const char *, ...);
void		 report_va_list(struct reporter *, enum report_type, const char *, va_list);

#endif /* !__REPORT_H_ */
