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
#include "heap.h"
#include "cm_quantile.h"

/* Static declarations */
static int cm_add_to_buffer(cm_quantile *cm, double value);
static int cm_cursor_increment(cm_quantile *cm);
static int cm_increment_insert_cursor(cm_quantile *cm);
static int cm_increment_compress_cursor(cm_quantile *cm);
static int cm_calc_rank_diff(cm_quantile *cm, uint64_t rank);

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
 * @arg error_epsilon The maximum error for the quantiles
 * @arg quantiles A sorted array of double quantile values, must be on (0, 1)
 * @arg num_quants The number of entries in the quantiles array
 * @arg cm_quantile The cm_quantile struct to initialize
 * @return 0 on success.
 */
int init_cm_quantile(double error_epsilon, double *quantiles, uint32_t num_quants, cm_quantile *cm) {
    // Verify the sanity of epsilon
    if (error_epsilon <= 0 or error_epsilon >= 0.5) return -1;

    // Verify the quantiles
    if (!num_quants) return -1;
    for (int i=0; i < num_quants; i++) {
        double val = quantiles[i];
        if (val <= 0 or val >= 1) return -1;
    }

    // Check that we have a non-null cm
    if (!cm) return -1;

    // Copy things
    cm->error_epsilon = error_epsilon;
    cm->quantiles = malloc(num_quants * sizeof(double));
    memcpy(cm->quantiles, quantiles, num_quants * sizeof(double));
    cm->num_quantiles = num_quants;
    cm->num_samples = 0;
    cm->samples = 0;

    // Initialize the buffer
    heap_create(&cm->buffer, 0, compare_double_keys);

    // Setup the cursors
    cm->insert.current = NULL;
    cm->insert.rank = 0;
    cm->compress.current = NULL;

    return 0;
}

/*
 * Callback function to delete the sample inside the
 * heap buffer.
 */
static void free_buffer_sample(void *key, void *val) {
    // The value is a cm_sample struct, so we can
    // just free it.
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
    cm->quantiles = NULL;

    // Destroy everything in the buffer
    heap_foreach(&cm->buffer, free_buffer_sample);
    heap_destroy(&cm->buffer);

    // Iterate through the linked list, free all
    cm_sample *next;
    cm_sample *current = cm->samples;
    while (current) {
        next = current->next;
        free(current);
        current = next;
    }
    cm->samples = NULL;

    return 0;
}

/**
 * Adds a new sample to the struct
 * @arg cm_quantile The cm_quantile to add to
 * @arg sample The new sample value
 * @return 0 on success.
 */
int cm_add_sample(cm_quantile *cm, double sample) {
    // Add to the buffer
    int res = 0;
    res = cm_add_to_buffer(cm, sample);
    if (res) return res;

    // Increment the cursors to amortize the work
    res = cm_increment_insert_cursor(cm);
    if (res) return res;

    res = cm_increment_compress_cursor(cm);
    if (res) return res;

    return res;
}

/**
 * Adds a new sample to the struct
 * @arg cm_quantile The cm_quantile to add to
 * @arg quantile The quantile to query
 * @return 0 on success.
 */
int cm_query(cm_quantile *cm, double quantile) {
    return 0;
}

/**
 * Adds a new sample to the buffer
 */
static int cm_add_to_buffer(cm_quantile *cm, double value) {
    // Allocate a new sample
    cm_sample *s = malloc(sizeof(cm_sample));
    s->value = value;
    s->next = NULL;

    // Add to the buffer
    heap_insert(&cm->buffer, &s->value, s);
    return 0;
}

/**
 * Computes the number of items to process in
 * one round of iteration
 */
static int cm_cursor_increment(cm_quantile *cm) {
    return ceil((cm->num_samples / 2) * cm->error_epsilon);
}


/**
 * Returns the smallest sample from the buffer or NULL if none.
 */
static cm_sample* cm_get_next_buffered(cm_quantile *cm) {
    void *key;
    void *val = NULL;
    heap_min(&cm->buffer, &key, &val);
    return val;
}

/**
 * Returns the smallest sample from the buffer or NULL if none,
 * and removes the sample from the buffer.
 */
static cm_sample* cm_delete_next_buffered(cm_quantile *cm) {
    void *key;
    void *val = NULL;
    heap_delmin(&cm->buffer, &key, &val);
    return val;
}

/**
 * Incrementally processes inserts by moving
 * data from the buffer to the samples using a cursor
 */
static int cm_increment_insert_cursor(cm_quantile *cm) {
    // Prepare to sample the buffer
    cm_sample *sample = cm_get_next_buffered(cm);
    if (!sample) return 0;
    double val = sample->value;

    // Check if this is the first sample or the smallest
    if (!cm->samples or val < cm->samples->value) {
        sample->next = cm->samples;
        sample->min_rank = 1;
        sample->rank_diff = 0;
        cm->samples = sample;
        cm->num_samples++;
        cm_delete_next_buffered(cm);
        return 0;

    // Check if this is the last element
    } else if (!cm->samples->next) {
        cm->samples->next = sample;
        sample->min_rank = 1;
        sample->rank_diff = 0;
        sample->next = NULL;
        cm->num_samples++;
        cm_delete_next_buffered(cm);
        return 0;
    }

    // Get the number of tuples to process
    int num = cm_cursor_increment(cm);

    for (int i=0; i < num; i++) {
        // If we don't have a cursor, start at the beginning
        if (!cm->insert.current) {
            cm->insert.current = cm->samples;
            cm->insert.rank = 0;

        // Check if our cursor is past the min-value
        } else if (val >= cm->insert.current->value) {
            // Check if this is the largest value (e.g. at the end)
            if (!cm->insert.current->next) {
                cm->insert.current->next = sample;
                sample->min_rank = 1;
                sample->rank_diff = 0;
                cm->num_samples++;

                // Get the next sample
                cm_delete_next_buffered(cm);
                sample = cm_get_next_buffered(cm);
                if (!sample) return 0;
                val = sample->value;

            // Check if we should insert the sample
            } else if  (val < cm->insert.current->next->value) {
                sample->next = cm->insert.current->next;
                cm->insert.current->next = sample;
                sample->min_rank = 1;
                sample->rank_diff = cm_calc_rank_diff(cm, cm->insert.rank);

                // Get the next sample
                cm_delete_next_buffered(cm);
                sample = cm_get_next_buffered(cm);
                if (!sample) return 0;
                val = sample->value;

            } else {
                cm->insert.current = cm->samples;
                cm->insert.rank = 0;
            }
        }

        // Increment the rank
        cm->insert.rank += cm->insert.current->min_rank;

        // Move the cursor
        cm->insert.current = cm->insert.current->next;
    }
}

/**
 * Incrementally processes compresses using a cursor
 */
static int cm_increment_compress_cursor(cm_quantile *cm) {

}

