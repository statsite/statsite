#include <pthread.h>

#include "lifoq.h"

struct lq_entry {
    size_t size;
    bool should_free: 1;
    void* data;
    struct lq_entry* next;
};

struct lifoq {
    struct lq_entry* head;
    size_t max_size;
    size_t cur_size;
    unsigned long overflows;
    bool closed;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

int lifoq_new(struct lifoq** q, size_t max_size) {
    *q = calloc(1, sizeof(struct lifoq));
    (**q).max_size = max_size;
    pthread_mutex_init(&(**q).mutex, NULL);
    pthread_cond_init(&(**q).cond, NULL);
    return 0;
}

/*
 * Remove the first entry of the queue, which requires a full linked
 * list traversal
 */
static void remove_tail(struct lifoq* q) {
    struct lq_entry* qe = q->head;
    struct lq_entry* qel = NULL;
    while(qe->next != NULL) {
        qel = qe;
        qe = qe->next;
    }
    if (qe->should_free)
        free(qe->data);
    q->cur_size -= qe->size;
    if (qel)
        qel->next = NULL;
    return;
}

/*
 * Put a qe_entry at the tail of the queue, where it will be the last
 * to be enqueued
 */
static void append(struct lifoq* q, struct lq_entry* e) {
    struct lq_entry* le = q->head;
    while (le->next != NULL)
        le = le->next;

    le->next = e;
}

int lifoq_close(struct lifoq* q) {
    int ret = 0;

    if (pthread_mutex_lock(&q->mutex))
        return LIFOQ_INTERNAL_ERROR;

    if (q->closed)
        ret = LIFOQ_ALREADY_CLOSED;
    q->closed = true;

    if (pthread_mutex_unlock(&q->mutex))
        ret = LIFOQ_INTERNAL_ERROR;
    return ret;
}

int lifoq_get(struct lifoq* q, void** data, size_t* size) {
    for(;;) {
        pthread_mutex_lock(&q->mutex);
        if (q->head) {
            struct lq_entry* qe = q->head;
            q->head = qe->next;
            q->cur_size -= qe->size;
            pthread_mutex_unlock(&q->mutex);
            *data = qe->data;
            *size = qe->size;
            free(qe);
            return 0;
        } else if (q->closed) {
            *data = NULL;
            *size = 0;
            pthread_mutex_unlock(&q->mutex);
            return LIFOQ_CLOSED;
        }
        pthread_cond_wait(&q->cond, &q->mutex);
    }
}

int lifoq_push(struct lifoq* q, void* data, size_t size, bool should_free) {
    int ret = 0;

    if (size > q->max_size)
        return LIFOQ_INVALID_ENTRY;

    if (pthread_mutex_lock(&q->mutex))
        return LIFOQ_INTERNAL_ERROR; /* Lock error */

    if (q->closed) { /* If the queue is closed no more pushes are possible */
        ret = LIFOQ_CLOSED;
        goto exit;
    }

    while (size + q->cur_size > q->max_size) {
        remove_tail(q);
        q->overflows++;
    }

    struct lq_entry* last_head = q->head;
    q->head = calloc(1, sizeof(struct lq_entry));
    q->head->data = data;
    q->head->should_free = should_free;
    q->head->next = last_head;
    q->head->size = size;
    q->cur_size += size;
    pthread_cond_signal(&q->cond);

exit:
    if (pthread_mutex_unlock(&q->mutex))
        ret = LIFOQ_INTERNAL_ERROR;
    return ret;
}
