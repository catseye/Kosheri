/*
 * portray.h
 * Prototypes and structures for rendering values to processes.
 * $Id: portray.h 107 2006-03-16 23:55:56Z catseye $
 */

#ifndef __PORTRAY_H_
#define __PORTRAY_H_

struct value;
struct process;

void		 value_portray(struct process *, struct value *);

#endif /* !__PORTRAY_H_ */
