#ifndef _STRBUF_H_
#define _STRBUF_H_

#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct strbuf strbuf;

/**
 * Allocate a new growable string buffer. The underlying buffer is also
 * contiguous, so may involve copies on resize.
 */
extern int strbuf_new(strbuf** buf, size_t initial_size);

/**
 * Free a growable buffer. If data_too is set to true, the undelrying
 * character buffer is also freed.
 */
extern void strbuf_free(strbuf* buf, bool data_too);

/**
 * Returns the underlying character buffer and the current strlen() value
 * of the buffer.
 */
extern char* strbuf_get(strbuf* buf, int* len);

/**
 * Append a sprintf value into the buffer.
 */
extern int strbuf_catsprintf(strbuf* buf, const char* fmt, ...);

/**
 * Concatenate the given string with the given length,
 * which is more efficient than calling catsprintf
 */
extern int strbuf_cat(strbuf* buf, const char* inp, int len);

#endif
