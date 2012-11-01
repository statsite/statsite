#include <math.h>
#include "counter.h"

/**
 * Initializes the counter struct
 * @arg counter The counter struct to initialize
 * @return 0 on success.
 */
int init_counter(counter *counter) {
    counter->count = 0;
    counter->sum = 0;
    counter->squared_sum = 0;
    counter->min = 0;
    counter->max = 0;
    return 0;
}

/**
 * Adds a new sample to the struct
 * @arg counter The counter to add to
 * @arg sample The new sample value
 * @return 0 on success.
 */
int counter_add_sample(counter *counter, double sample) {
    if (counter->count == 0) {
        counter->min = counter->max = sample;
    } else {
        if (counter->min > sample)
            counter->min = sample;
        else if (counter->max < sample)
            counter->max = sample;
    }
    counter->count++;
    counter->sum += sample;
    counter->squared_sum += pow(sample, 2);
    return 0;
}

/**
 * Returns the number of samples in the counter
 * @arg counter The counter to query
 * @return The number of samples
 */
uint64_t counter_count(counter *counter) {
    return counter->count;
}

/**
 * Returns the mean counter value
 * @arg counter The counter to query
 * @return The mean value
 */
double counter_mean(counter *counter) {
    return (counter->count) ? counter->sum / counter->count : 0;
}

/**
 * Returns the sample standard deviation counter value
 * @arg counter The counter to query
 * @return The sample standard deviation
 */
double counter_stddev(counter *counter) {
    double num = (counter->count * counter->squared_sum) - pow(counter->sum, 2);
    double div = counter->count * (counter->count - 1);
    if (div == 0) return 0;
    return sqrt(num / div);
}

/**
 * Returns the sum of the counter
 * @arg counter The counter to query
 * @return The sum of values
 */
double counter_sum(counter *counter) {
    return counter->sum;
}

/**
 * Returns the sum squared of the counter
 * @arg counter The counter to query
 * @return The sum squared of values
 */
double counter_squared_sum(counter *counter) {
    return counter->squared_sum;
}

/**
 * Returns the minimum value of the counter
 * @arg counter The counter to query
 * @return The minimum value.
 */
double counter_min(counter *counter) {
    return counter->min;
}

/**
 * Returns the maximum value of the counter
 * @arg counter The counter to query
 * @return The maximum value.
 */
double counter_max(counter *counter) {
    return counter->max;
}

