/**
 * This module implements the Cormode-Muthukrishnan algorithm
 * for computation of biased quantiles over data streams from
 * "Effective Computation of Biased Quantiles over Data Streams"
 *
 */
#ifndef CM_QUANTILE_H
#define CM_QUANTILE_H

typedef struct cm_sample {
    double value;
    uint64_t min_rank;
    uint64_t rank_diff;
    struct cm_sample *next;
} cm_sample;

struct cm_insert_cursor {
    cm_sample *current;
    uint64_t rank;
};

struct cm_compress_cursor {
    cm_sample *current;
};

typedef struct {
    double error_epsilon;  // Desired epsilon
    double *quantiles;      // Queryable quantiles, sorted array
    uint32_t num_quantiles; // Number of quantiles
    uint64_t num_samples;   // Number of samples
    uint64_t num_values;    // Number of values added
    cm_sample *samples;     // Sorted linked list of samples
    heap buffer;     // Sample buffer
    struct cm_insert_cursor insert;     // Insertion cursor
    struct cm_compress_cursor compress; // Compression cursor
} cm_quantile;


/**
 * Initializes the CM quantile struct
 * @arg error_epsilon The maximum error for the quantiles
 * @arg quantiles A sorted array of double quantile values, must be on (0, 1)
 * @arg num_quants The number of entries in the quantiles array
 * @arg cm_quantile The cm_quantile struct to initialize
 * @return 0 on success.
 */
int init_cm_quantile(double error_epsilon, double *quantiles, uint32_t num_quants, cm_quantile *cm);

/**
 * Destroy the CM quantile struct.
 * @arg cm_quantile The cm_quantile to destroy
 * @return 0 on success.
 */
int destroy_cm_quantile(cm_quantile *cm);

/**
 * Adds a new sample to the struct
 * @arg cm_quantile The cm_quantile to add to
 * @arg sample The new sample value
 * @return 0 on success.
 */
int cm_add_sample(cm_quantile *cm, double sample);

/**
 * Adds a new sample to the struct
 * @arg cm_quantile The cm_quantile to add to
 * @arg quantile The quantile to query
 * @return 0 on success.
 */
int cm_query(cm_quantile *cm, double quantile);

#endif
