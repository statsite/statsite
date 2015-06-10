#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "strbuf.h"

#define GROWTH_FACTOR 1.3
#define MINIMUM_GROWTH 128

struct strbuf {
    char* d; /* The main data pointer */
    size_t size; /* The size of the buffer */
    int len;  /* The strlen() of the buffer */
};

void expand(strbuf* buf, size_t new_size) {
    if (new_size == 0)
        new_size = buf->size * GROWTH_FACTOR;
    else
        new_size += buf->size;

    if (new_size <= buf->size)
        new_size += MINIMUM_GROWTH;

    buf->d = realloc(buf->d, new_size);
    buf->size = new_size;
}

int strbuf_new(strbuf** buf, size_t initial_size) {
    *buf = malloc(sizeof(strbuf));
    if (initial_size == 0)
        initial_size = MINIMUM_GROWTH;

    (*buf)->size = initial_size;
    (*buf)->len = 0;
    (*buf)->d = malloc(initial_size);
    *(*buf)->d = '\0';
    return 0;
}

void strbuf_free(strbuf* buf, bool data_too) {
    if (data_too)
        free(buf->d);
    free(buf);
}

char* strbuf_get(strbuf* buf, int* len) {
    *len = buf->len;
    return buf->d;
}

int strbuf_catsprintf(strbuf* buf, const char* fmt, ...) {
    va_list args;

    while(true) {
        size_t available = buf->size - buf->len - 1;
        va_start(args, fmt);
        size_t printed = vsnprintf(&(buf->d[buf->len]), available, fmt, args);
        va_end(args);

        if (printed < available) {
            buf->len += printed;
            return buf->len;
        }

        expand(buf, printed + 1);
    }


    return 0;
}

int strbuf_cat(strbuf* buf, const char* inp, int len) {
    if (len + buf->len >= buf->size)
        expand(buf, len + 1);
    memcpy(&buf->d[buf->len], inp, len);
    buf->len += len;
    buf->d[buf->len] = '\0';
    return buf->len;
}
