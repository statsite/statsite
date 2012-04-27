#include <stdlib.h>
#include <string.h>
#include "metrics.h"

static int counter_delete_cb(void *data, const char *key, void *value);
static int timer_delete_cb(void *data, const char *key, void *value);

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
int destroy_metrics(metrics *m) {
    // Clear the copied quantiles array
    free(m->quantiles);

    // Nuke all the k/v pairs
    key_val *current = m->kv_vals;
    key_val *prev = NULL;
    while (current) {
        free(current->name);
        prev = current;
        current = current->next;
        free(prev);
    }

    // Nuke the counters
    hashmap_iter(m->counters, counter_delete_cb, NULL);
    hashmap_destroy(m->counters);

    // Nuke the timers
    hashmap_iter(m->timers, timer_delete_cb, NULL);
    hashmap_destroy(m->timers);

    return 0;
}

/**
 * Increments the counter with the given name
 * by a value.
 * @arg name The name of the counter
 * @arg val The value to add
 * @return 0 on success
 */
int metrics_increment_counter(metrics *m, char *name, double val) {
    counter *c;
    int res = hashmap_get(m->counters, name, (void**)&c);

    // New counter
    if (res == -1) {
        c = malloc(sizeof(counter));
        init_counter(c);
        hashmap_put(m->counters, name, c);
    }

    // Add the sample value
    return counter_add_sample(c, val);
}

/**
 * Adds a new timer sample for the timer with a
 * given name.
 * @arg name The name of the timer
 * @arg val The sample to add
 * @return 0 on success.
 */
int metrics_add_timer_sample(metrics *m, char *name, double val) {
    timer *t;
    int res = hashmap_get(m->timers, name, (void**)&t);

    // New timer
    if (res == -1) {
        t = malloc(sizeof(timer));
        init_timer(m->eps, m->quantiles, m->num_quants, t);
        hashmap_put(m->timers, name, t);
    }

    // Add the sample value
    return timer_add_sample(t, val);
}

/**
 * Adds a new K/V pair
 * @arg name The key name
 * @arg val The value associated
 * @return 0 on success.
 */
int metrics_add_kv(metrics *m, char *name, double val) {
    key_val *kv = malloc(sizeof(key_val));
    kv->name = strdup(name);
    kv->val = val;
    kv->next = m->kv_vals;
    m->kv_vals = kv;
    return 0;
}


// Counter map cleanup
static int counter_delete_cb(void *data, const char *key, void *value) {
    free(value);
    return 0;
}

// Timer map cleanup
static int timer_delete_cb(void *data, const char *key, void *value) {
    timer *t = value;
    destroy_timer(t);
    free(t);
    return 0;
}

