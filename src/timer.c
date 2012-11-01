#include <math.h>
#include "timer.h"

/* Static declarations */
static void finalize_timer(timer *timer);

/**
 * Initializes the timer struct
 * @arg eps The maximum error for the quantiles
 * @arg quantiles A sorted array of double quantile values, must be on (0, 1)
 * @arg num_quants The number of entries in the quantiles array
 * @arg timeer The timer struct to initialize
 * @return 0 on success.
 */
int init_timer(double eps, double *quantiles, uint32_t num_quants, timer *timer) {
    timer->count = 0;
    timer->sum = 0;
    timer->squared_sum = 0;
    timer->finalized = 1;
    int res = init_cm_quantile(eps, quantiles, num_quants, &timer->cm);
    return res;
}

/**
 * Destroy the timer struct.
 * @arg timer The timer to destroy
 * @return 0 on success.
 */
int destroy_timer(timer *timer) {
    return destroy_cm_quantile(&timer->cm);
}

/**
 * Adds a new sample to the struct
 * @arg timer The timer to add to
 * @arg sample The new sample value
 * @return 0 on success.
 */
int timer_add_sample(timer *timer, double sample) {
    timer->count += 1;
    timer->sum += sample;
    timer->squared_sum += pow(sample, 2);
    timer->finalized = 0;
    return cm_add_sample(&timer->cm, sample);
}

/**
 * Queries for a quantile value
 * @arg timer The timer to query
 * @arg quantile The quantile to query
 * @return The value on success or 0.
 */
double timer_query(timer *timer, double quantile) {
    finalize_timer(timer);
    return cm_query(&timer->cm, quantile);
}

/**
 * Returns the number of samples in the timer
 * @arg timer The timer to query
 * @return The number of samples
 */
uint64_t timer_count(timer *timer) {
    return timer->count;
}

/**
 * Returns the minimum timer value
 * @arg timer The timer to query
 * @return The number of samples
 */
double timer_min(timer *timer) {
    finalize_timer(timer);
    if (!timer->cm.samples) return 0;
    return timer->cm.samples->value;
}

/**
 * Returns the mean timer value
 * @arg timer The timer to query
 * @return The mean value
 */
double timer_mean(timer *timer) {
    return (timer->count) ? timer->sum / timer->count : 0;
}

/**
 * Returns the sample standard deviation timer value
 * @arg timer The timer to query
 * @return The sample standard deviation
 */
double timer_stddev(timer *timer) {
    double num = (timer->count * timer->squared_sum) - pow(timer->sum, 2);
    double div = timer->count * (timer->count - 1);
    if (div == 0) return 0;
    return sqrt(num / div);
}

/**
 * Returns the sum of the timer
 * @arg timer The timer to query
 * @return The sum of values
 */
double timer_sum(timer *timer) {
    return timer->sum;
}

/**
 * Returns the sum squared of the timer
 * @arg timer The timer to query
 * @return The sum squared of values
 */
double timer_squared_sum(timer *timer) {
    return timer->squared_sum;
}

/**
 * Returns the maximum timer value
 * @arg timer The timer to query
 * @return The maximum value
 */
double timer_max(timer *timer) {
    finalize_timer(timer);
    if (!timer->cm.end) return 0;
    return timer->cm.end->value;
}

// Finalizes the timer for queries
static void finalize_timer(timer *timer) {
    if (timer->finalized) return;

    // Force the quantile to flush internal
    // buffers so that queries are accurate.
    cm_flush(&timer->cm);

    timer->finalized = 1;
}

