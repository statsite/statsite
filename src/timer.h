#ifndef TIMER_H
#define TIMER_H
#include <stdint.h>
#include "cm_quantile.h"

typedef struct {
    uint64_t count;     // Count of items
    double sum;         // Sum of the values
    double squared_sum; // Sum of the squared values
    int finalized;      // Is the cm_quantile finalized
    cm_quantile cm;     // Quantile we use
} timer;

/**
 * Initializes the timer struct
 * @arg eps The maximum error for the quantiles
 * @arg quantiles A sorted array of double quantile values, must be on (0, 1)
 * @arg num_quants The number of entries in the quantiles array
 * @arg timeer The timer struct to initialize
 * @return 0 on success.
 */
int init_timer(double eps, double *quantiles, uint32_t num_quants, timer *timer);

/**
 * Destroy the timer struct.
 * @arg timer The timer to destroy
 * @return 0 on success.
 */
int destroy_timer(timer *timer);

/**
 * Adds a new sample to the struct
 * @arg timer The timer to add to
 * @arg sample The new sample value
 * @return 0 on success.
 */
int timer_add_sample(timer *timer, double sample);

/**
 * Queries for a quantile value
 * @arg timer The timer to query
 * @arg quantile The quantile to query
 * @return The value on success or 0.
 */
double timer_query(timer *timer, double quantile);

/**
 * Returns the number of samples in the timer
 * @arg timer The timer to query
 * @return The number of samples
 */
uint64_t timer_count(timer *timer);

/**
 * Returns the minimum timer value
 * @arg timer The timer to query
 * @return The number of samples
 */
double timer_min(timer *timer);

/**
 * Returns the mean timer value
 * @arg timer The timer to query
 * @return The mean value
 */
double timer_mean(timer *timer);

/**
 * Returns the sample standard deviation timer value
 * @arg timer The timer to query
 * @return The sample standard deviation
 */
double timer_stddev(timer *timer);

/**
 * Returns the sum of the timer
 * @arg timer The timer to query
 * @return The sum of values
 */
double timer_sum(timer *timer);

/**
 * Returns the sum squared of the timer
 * @arg timer The timer to query
 * @return The sum squared of values
 */
double timer_squared_sum(timer *timer);

/**
 * Returns the maximum timer value
 * @arg timer The timer to query
 * @return The maximum value
 */
double timer_max(timer *timer);

#endif
