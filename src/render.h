/*
 * render.h
 * Prototypes for stream rendering (printf-like formatting).
 * $Id: stream.h 116 2007-03-26 00:43:44Z catseye $
 */

#ifndef __RENDER_H_
#define __RENDER_H_

struct process;

void		 process_render(struct process *, const char *, ...);

#endif	/* !__RENDER_H_ */
