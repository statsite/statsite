#include <math.h>
#include "counter.h"

/**
 * Initializes the counter struct
 * @arg counter The counter struct to initialize
 * @return 0 on success.
 */
int init_counter(counter *counter) {
    counter->sum = 0;
    return 0;
}

/**
 * Adds a new sample to the struct
 * @arg counter The counter to add to
 * @arg sample The new sample value
 * @return 0 on success.
 */
int counter_add_sample(counter *counter, double sample, double sample_rate) {
    counter->sum += sample;
    return 0;
}

double counter_sum(counter *counter) {
    return counter->sum;
}
