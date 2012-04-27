#include <stdlib.h>
#include <string.h>
#include "metrics.h"

/**
 * Initializes the metrics struct.
 * @arg eps The maximum error for the quantiles
 * @arg quantiles A sorted array of double quantile values, must be on (0, 1)
 * @arg num_quants The number of entries in the quantiles array
 * @return 0 on success.
 */
int init_metrics(double eps, double *quantiles, uint32_t num_quants, metrics *m) {
    // Copy the inputs
    m->eps = eps;
    m->num_quants = num_quants;
    m->quantiles = malloc(num_quants * sizeof(double));
    memcpy(m->quantiles, quantiles, num_quants * sizeof(double));

    // Allocate the hashmaps
    int res = hashmap_init(0, &m->counters);
    if (res) return res;
    res = hashmap_init(0, &m->timers);
    if (res) return res;

    // Set the head of our linked list to null
    m->kv_vals = NULL;
    return 0;
}

/**
 * Initializes the metrics struct, with preset configurations.
 * This defaults to a epsilon of 0.01 (1% error), and quantiles at
 * 0.5, 0.95, and 0.99.
 * @return 0 on success.
 */
int init_metrics_defaults(metrics *m) {
    double quants[] = {0.5, 0.95, 0.99};
    return init_metrics(0.01, (double*)&quants, 3, m);
}

/**
 * Destroys the metrics
 * @return 0 on success.
 */
int destroy_metrics(metrics *m);

/**
 * Increments the counter with the given name
 * by a value.
 * @arg name The name of the counter
 * @arg val The value to add
 * @return 0 on success
 */
int metrics_increment_counter(metrics *m, char *name, double val);

/**
 * Adds a new timer sample for the timer with a
 * given name.
 * @arg name The name of the timer
 * @arg val The sample to add
 * @return 0 on success.
 */
int metrics_add_timer_sample(metrics *m, char *name, double val);

/**
 * Adds a new K/V pair
 * @arg name The key name
 * @arg val The value associated
 * @return 0 on success.
 */
int metrics_add_kv(metrics *m, char *name, double val);


