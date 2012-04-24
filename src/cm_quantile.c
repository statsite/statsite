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
#include "heap.h"
#include "cm_quantile.h"

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
    return 0;
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

