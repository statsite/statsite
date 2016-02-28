/**
 * This module implements the Cormode-Muthukrishnan algorithm
 * for computation of biased quantiles over data streams from
 * "Effective Computation of Biased Quantiles over Data Streams"
 *
 */
#include <stdint.h>
#include <iso646.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include "heap.h"
#include "cm_quantile.h"

/* Static declarations */
static void cm_add_to_buffer(cm_quantile *cm, double value);
static double cm_insert_point_value(cm_quantile *cm);
static void cm_reset_insert_cursor(cm_quantile *cm);
static int cm_cursor_increment(cm_quantile *cm);
static void cm_insert_sample(cm_quantile *cm, cm_sample *position, cm_sample *new);
static void cm_append_sample(cm_quantile *cm, cm_sample *new);
static void cm_insert(cm_quantile *cm);
static void cm_compress(cm_quantile *cm);
static uint64_t cm_threshold(cm_quantile *cm, uint64_t rank);

// This is a comparison function that treats keys as doubles
static int compare_double_keys(register void* key1, register void* key2) {
    // Cast them as double* and read them in
    register double val1 = *((double*)key1);
    register double val2 = *((double*)key2);

    // Perform the comparison
    if (val1 < val2)
        return -1;
    else if (val1 == val2)
        return 0;
    else
        return 1;
}

/**
 * Initializes the CM quantile struct
 * @arg eps The maximum error for the quantiles
 * @arg quantiles A sorted array of double quantile values, must be on (0, 1)
 * @arg num_quants The number of entries in the quantiles array
 * @arg cm_quantile The cm_quantile struct to initialize
 * @return 0 on success.
 */
int init_cm_quantile(double eps, double *quantiles, uint32_t num_quants, cm_quantile *cm) {
    // Verify the sanity of epsilon
    if (eps <= 0 or eps >= 0.5) return -1;

    // Verify the quantiles
    if (!num_quants) return -1;
    for (int i=0; i < num_quants; i++) {
        double val = quantiles[i];
        if (val <= 0 or val >= 1) return -1;
    }

    // Check that we have a non-null cm
    if (!cm) return -1;

    // Initialize
    cm->eps = eps;
    cm->num_samples = 0;
    cm->num_values = 0;
    cm->samples = NULL;
    cm->end = NULL;

    // Copy the quantiles
    cm->quantiles = malloc(num_quants * sizeof(double));
    memcpy(cm->quantiles, quantiles, num_quants * sizeof(double));
    cm->num_quantiles = num_quants;

    // Initialize the buffers
    heap *heaps = malloc(2*sizeof(heap));
    cm->bufLess = heaps;
    cm->bufMore = heaps+1;
    heap_create(cm->bufLess, 0, compare_double_keys);
    heap_create(cm->bufMore, 0, compare_double_keys);

    // Setup the cursors
    cm->insert.curs = NULL;
    cm->compress.curs = NULL;

    return 0;
}

/*
 * Callback function to delete the sample inside the
 * heap buffer.
 */
static void free_buffer_sample(void *key, void *val) {
    free(val);
}

/**
 * Destroy the CM quantile struct.
 * @arg cm_quantile The cm_quantile to destroy
 * @return 0 on success.
 */
int destroy_cm_quantile(cm_quantile *cm) {
    // Free the quantiles
    free(cm->quantiles);

    // Destroy everything in the buffer
    heap_foreach(cm->bufLess, free_buffer_sample);
    heap_destroy(cm->bufLess);
    heap_foreach(cm->bufMore, free_buffer_sample);
    heap_destroy(cm->bufMore);

    // Free the lower address, since they are allocated to be adjacent
    free((cm->bufLess < cm->bufMore) ? cm->bufLess : cm->bufMore);

    // Iterate through the linked list, free all
    cm_sample *next;
    cm_sample *current = cm->samples;
    while (current) {
        next = current->next;
        free(current);
        current = next;
    }

    return 0;
}

/**
 * Adds a new sample to the struct
 * @arg cm_quantile The cm_quantile to add to
 * @arg sample The new sample value
 * @return 0 on success.
 */
int cm_add_sample(cm_quantile *cm, double sample) {
    cm_add_to_buffer(cm, sample);
    cm_insert(cm);
    cm_compress(cm);
    return 0;
}

/**
 * Forces the internal buffers to be flushed,
 * this allows query to have maximum accuracy.
 * @arg cm_quantile The cm_quantile to add to
 * @return 0 on success.
 */
int cm_flush(cm_quantile *cm) {
    int rounds = 0;
    while (heap_size(cm->bufLess) or heap_size(cm->bufMore)) {
        if (heap_size(cm->bufMore) == 0) cm_reset_insert_cursor(cm);
        cm_insert(cm);
        cm_compress(cm);
        rounds++;
    }
    return 0;
}

/**
 * Queries for a quantile value
 * @arg cm_quantile The cm_quantile to query
 * @arg quantile The quantile to query
 * @return The value on success or 0.
 */
double cm_query(cm_quantile *cm, double quantile) {
    uint64_t rank = ceil(quantile * cm->num_values);
	uint64_t min_rank=0;
    uint64_t max_rank;
	uint64_t threshold = ceil(cm_threshold(cm, rank) / 2.);

    cm_sample *prev = cm->samples;
    cm_sample *current = cm->samples;
    while (current) {
        max_rank = min_rank + current->width + current->delta;
        if (max_rank > rank + threshold || min_rank > rank) {
            break;
        }
        min_rank += current->width;
        prev = current;
        current = current->next;
    }
    return (prev) ? prev->value : 0;
}

/**
 * Adds a new sample to the buffer
 */
static void cm_add_to_buffer(cm_quantile *cm, double value) {
    // Allocate a new sample
    cm_sample *s = calloc(1, sizeof(cm_sample));
    s->value = value;

    /*
     * Check the cursor value.
     * Only use bufLess if we have at least a single value.
     */
    if (cm->num_values && value < cm_insert_point_value(cm)) {
        heap_insert(cm->bufLess, &s->value, s);
    } else {
        heap_insert(cm->bufMore, &s->value, s);
    }
}

// Returns the value under the insertion cursor or 0
static double cm_insert_point_value(cm_quantile *cm) {
    return (cm->insert.curs) ? cm->insert.curs->value : 0;
}

// Resets the insert cursor
static void cm_reset_insert_cursor(cm_quantile *cm) {
    // Swap the buffers, reset the cursor
    heap *tmp = cm->bufLess;
    cm->bufLess = cm->bufMore;
    cm->bufMore = tmp;
    cm->insert.curs = NULL;
}

// Computes the number of items to process in one iteration
static int cm_cursor_increment(cm_quantile *cm) {
    return ceil(cm->num_samples * cm->eps);
}

/* Inserts a new sample before the position sample */
static void cm_insert_sample(cm_quantile *cm, cm_sample *position, cm_sample *new) {
    // Inserting at the head
    if (!position->prev) {
        position->prev = new;
        cm->samples = new;
        new->next = position;
    } else {
        cm_sample *prev = position->prev;
        prev->next = new;
        position->prev = new;
        new->prev = prev;
        new->next = position;
    }
}

/* Inserts a new sample at the end */
static void cm_append_sample(cm_quantile *cm, cm_sample *new) {
    new->prev = cm->end;
    cm->end->next = new;
    cm->end = new;
}

/**
 * Incrementally processes inserts by moving
 * data from the buffer to the samples using a cursor
 */
static void cm_insert(cm_quantile *cm) {
    // Check if this is the first element
    cm_sample *samp;
    if (!cm->samples) {
        if (!heap_delmin(cm->bufMore, NULL, (void**)&samp)) return;
        samp->width = 1;
        samp->delta = 0;
        cm->samples = samp;
        cm->end = samp;
        cm->num_values++;
        cm->num_samples++;
        cm->insert.curs = samp;
		return;
	}

    // Check if we need to initialize the cursor
    if (!cm->insert.curs) {
        cm->insert.curs = cm->samples;
    }

    // Handle adding values in the middle
    int incr_size = cm_cursor_increment(cm);
    double *val;
    for (int i=0; i < incr_size and cm->insert.curs; i++) {
        while (heap_min(cm->bufMore, (void**)&val, NULL) && *val <= cm_insert_point_value(cm)) {
            heap_delmin(cm->bufMore, NULL, (void**)&samp);
            samp->width = 1;
            samp->delta = cm->insert.curs->width + cm->insert.curs->delta - 1;
            cm_insert_sample(cm, cm->insert.curs, samp);
            cm->num_values++;
            cm->num_samples++;

            // Check if we need to update the compress cursor
            if (cm->compress.curs && cm->compress.curs->value >= samp->value) {
                cm->compress.min_rank++;
            }
        }
        // Increment the cursor
        cm->insert.curs = cm->insert.curs->next;
	}

    // Handle adding values at the end
    if (cm->insert.curs == NULL) {
        while (heap_min(cm->bufMore, (void**)&val, NULL) && *val > cm->end->value) {
            heap_delmin(cm->bufMore, NULL, (void**)&samp);
            samp->width = 1;
            samp->delta = 0;
            cm_append_sample(cm, samp);
            cm->num_values++;
            cm->num_samples++;
        }

        // Reset the cursor
        cm_reset_insert_cursor(cm);
	}
}

/* Incrementally processes compression by using a cursor */
static void cm_compress(cm_quantile *cm) {
    // Bail early if there is nothing to really compress..
    if (cm->num_samples < 3) return;

    // Check if we need to initialize the cursor
    if (!cm->compress.curs) {
        cm->compress.curs = cm->end->prev;
        cm->compress.min_rank = cm->num_values - 1 - cm->compress.curs->width;
        cm->compress.curs = cm->compress.curs->prev;
    }

    int incr_size = cm_cursor_increment(cm);
    cm_sample *next, *prev;
    uint64_t threshold;
    uint64_t max_rank, test_val;
    for (int i=0; i < incr_size and cm->compress.curs != cm->samples; i++) {
        next = cm->compress.curs->next;
        max_rank = cm->compress.min_rank + cm->compress.curs->width + cm->compress.curs->delta;
        cm->compress.min_rank -= cm->compress.curs->width;

        threshold = cm_threshold(cm, max_rank);
        test_val = cm->compress.curs->width + next->width + next->delta;
        if (test_val <= threshold) {
            // Make sure we don't stomp the insertion cursor
            if (cm->insert.curs == cm->compress.curs) {
                cm->insert.curs = next;
            }

            // Combine the widths
            next->width += cm->compress.curs->width;

            // Remove the tuple
            prev = cm->compress.curs->prev;
            prev->next = next;
            next->prev = prev;
            free(cm->compress.curs);
            cm->compress.curs = prev;

            // Reduce the sample count
            cm->num_samples--;
        } else {
            cm->compress.curs = cm->compress.curs->prev;
        }
    }

    // Reset the cursor if we hit the start
    if (cm->compress.curs == cm->samples) cm->compress.curs = NULL;
}

/* Computes the minimum threshold value */
static uint64_t cm_threshold(cm_quantile *cm, uint64_t rank) {
    uint64_t min_val = LLONG_MAX;

    uint64_t quant_min;
    double   quant;
    for (int i=0; i < cm->num_quantiles; i++) {
        quant = cm->quantiles[i];
        if (rank >= quant * cm->num_values) {
            quant_min = 2 * cm->eps * rank / quant;
        } else {
            quant_min = 2 * cm->eps * (cm->num_values - rank) / (1 - quant);
        }
        if (quant_min < min_val) min_val = quant_min;
    }

    return min_val;
}

