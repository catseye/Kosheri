/*
 * stream.h
 * Routines for communicating with stream-like processes.
 */

#ifndef __STREAM_H_
#define __STREAM_H_

struct process;

/*
 * The first argument is the process that is calling the stream-
 * like process, a.k.a. "self".  It may be NULL, to communicate
 * with a stream from a non-process context.
 */

/*
 * Send a message of the form <write: data-in-symbol-form> to the stream.
 */
void		 stream_write(struct process *, struct process *, const void *, unsigned int);

/*
 * Send a message of the form <read: receiver, length> to the stream.
 * If self was given, it will be used as the receiver, and it will receive
 * a response message in the form of a symbol.
 * If self is NULL, a provisional pseudo-receiver process will be supplied
 * by this function, and data will be returned in the void *.
 */
void		 stream_read(struct process *, struct process *, void *, unsigned int);

/*
 * Send a message of the form <eof: receiver> to the stream.
 * If self was given, it will be used as the receiver, and it will receive
 * a response message in the form of a boolean.
 * If self is NULL, a provisional pseudo-receiver process will be supplied
 * by this function, and the boolean will be returned by the function.
 */
int		 stream_is_at_end(struct process *, struct process *);

/*
 * Send a message of the form <close:> to the stream.
 */ 
void		 stream_close(struct process *, struct process *);

#endif /* !__FILE_H_ */
