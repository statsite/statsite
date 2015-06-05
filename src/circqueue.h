#ifndef _CIRCQUEUE_H_
#define _CIRCQUEUE_H_

#include <stdint.h>
#include <sys/uio.h>

/**
 * Represents a simple circular buffer
 */
typedef struct {
    int write_cursor;
    int read_cursor;
    uint32_t buf_size;
    char *buffer;
} circular_buffer;

// Circular buffer method
void circbuf_init(circular_buffer *buf);
void circbuf_clear(circular_buffer *buf);
void circbuf_free(circular_buffer *buf);
uint64_t circbuf_avail_buf(circular_buffer *buf);
uint64_t circbuf_used_buf(circular_buffer *buf);
void circbuf_grow_buf(circular_buffer *buf);
void circbuf_setup_readv_iovec(circular_buffer *buf, struct iovec *vectors, int *num_vectors);
void circbuf_advance_write(circular_buffer *buf, uint64_t bytes);
void circbuf_advance_read(circular_buffer *buf, uint64_t bytes);
int circbuf_write(circular_buffer *buf, char *in, uint64_t bytes);

#endif
