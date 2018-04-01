#ifndef COUNTER_H
#define COUNTER_H
#include <stdint.h>

typedef struct {
    double sum;            // Sum of the values
} counter;

/**
 * Initializes the counter struct
 * @arg counter The counter struct to initialize
 * @return 0 on success.
 */
int init_counter(counter *counter);

/**
 * Adds a new sample to the struct
 * @arg counter The counter to add to
 * @arg sample The new sample value
 * @return 0 on success.
 */
int counter_add_sample(counter *counter, double sample, double sample_rate);

/**
 * Returns the sum of the counter
 * @arg counter The counter to query
 * @return The sum of values
 */
double counter_sum(counter *counter);

#endif
