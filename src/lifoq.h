#ifndef _LIFOQ_H_
#define _LIFOQ_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct lifoq lifoq;

enum {
    LIFOQ_INTERNAL_ERROR = -1,
    LIFOQ_INVALID_ENTRY = -2,
    LIFOQ_CLOSED = -3,
    LIFOQ_ALREADY_CLOSED = -4,
    LIFOQ_FULL = -5
};

/**
 * Initialize a new LIFO queue, which contains up to max_size entries.
 * Sizes are arbitrary and are specified when data is added.
 */
extern int lifoq_new(lifoq** q, size_t max_size);

/**
 * Add an entry to the lifoq, with data provided and size given.
 * should_free specifies if, upon removing the data due to queue
 * overflow, it should be free()ed.
 *
 * If this function returns a non-zero error state, data will not be
 * free()ed.
 */
extern int lifoq_push(lifoq* q, void* data, size_t size, bool should_free, bool fail_full);

/**
 * Get an entry from the queue. Place the result in data, and return the
 * original size in size. If no queue entries are available, block.
 * If the lifoq is marked as closed, this function will continue to
 * provide data, but will return LIFOQ_CLOSED when it is empty.
 */
extern int lifoq_get(lifoq* q, void** data, size_t* size);

/**
 * Close a lifoq. This will prevent any further pushes, but lifoq_get will
 * allow draining the queue until it is empty, where LIFQ_CLOSED will be returned.
 */
extern int lifoq_close(lifoq* q);

#endif
