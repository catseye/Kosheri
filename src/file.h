/*
 * file.h
 * Native processes for communicating with (reading, writing) files,
 * exposing a stream-like interface.
 */

#ifndef __FILE_H_
#define __FILE_H_

struct process;

struct process	*file_open(const char *, const char *);

#endif /* !__FILE_H_ */
