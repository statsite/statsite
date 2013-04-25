/*
 * Based on the Google paper
 * "HyperLogLog in Practice: Algorithmic Engineering of a
State of The Art Cardinality Estimation Algorithm"
 *
 * We implement a HyperLogLog using 6 bits for register,
 * and a 64bit hash function. For our needs, we always use
 * a dense representation and avoid the sparse/dense conversions.
 *
 */
#include <stdlib.h>
#include <math.h>
#include <strings.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "hll.h"
#include "hll_constants.h"

#define REG_WIDTH 6     // Bits per register
#define INT_WIDTH 32    // Bits in an int
#define REG_PER_WORD 5  // floor(INT_WIDTH / REG_WIDTH)

#define NUM_REG(precision) ((1 << precision))

// Link the external murmur hash in
extern void MurmurHash3_x64_128(const void * key, const int len, const uint32_t seed, void *out);


/**
 * Initializes a new HLL
 * @arg precision The digits of precision to use
 * @arg h The HLL to initialize
 * @return 0 on success
 */
int hll_init(unsigned char precision, hll_t *h) {
    // Ensure the precision is somewhat sane
    if (precision < HLL_MIN_PRECISION || precision > HLL_MAX_PRECISION)
        return -1;

    // Store precision
    h->precision = precision;

    // Determine how many registers are needed
    int reg = NUM_REG(precision);

    // Get the full words required
    int words = ceil(reg / (double)REG_PER_WORD);

    // Allocate and zero out the registers
    h->registers = calloc(words, sizeof(uint32_t));
    if (!h->registers) return -1;
    return 0;
}


/**
 * Destroys an hll
 * @return 0 on success
 */
int hll_destroy(hll_t *h) {
    free(h->registers);
    return 0;
}

static int get_register(hll_t *h, int idx) {
    uint32_t word = *(h->registers + (idx / REG_PER_WORD));
    word = word >> REG_WIDTH * (idx % REG_PER_WORD);
    return word & ((1 << REG_WIDTH) - 1);
}

static void set_register(hll_t *h, int idx, int val) {
    uint32_t *word = h->registers + (idx / REG_PER_WORD);

    // Shift the val into place
    unsigned shift = REG_WIDTH * (idx % REG_PER_WORD);
    val = val << shift;
    uint32_t val_mask = ((1 << REG_WIDTH) - 1) << shift;

    // Store the word
    *word = (*word & ~val_mask) | val;
}

/**
 * Adds a new key to the HLL
 * @arg h The hll to add to
 * @arg key The key to add
 */
void hll_add(hll_t *h, char *key) {
    // Compute the hash value of the key
    uint64_t out[2];
    MurmurHash3_x64_128(key, strlen(key), 0, &out);

    // Add the hashed value
    hll_add_hash(h, out[1]);
}

/**
 * Adds a new hash to the HLL
 * @arg h The hll to add to
 * @arg hash The hash to add
 */
void hll_add_hash(hll_t *h, uint64_t hash) {
    // Determine the index using the first p bits
    int idx = hash >> (64 - h->precision);

    // Shift out the index bits
    hash = hash << h->precision | (1 << (h->precision -1));

    // Determine the count of leading zeros
    int leading = __builtin_clzll(hash) + 1;

    // Update the register if the new value is larger
    if (leading > get_register(h, idx)) {
        set_register(h, idx, leading);
    }
}

/*
 * Returns the bias correctors from the
 * hyperloglog paper
 */
static double alpha(unsigned char precision) {
    switch (precision) {
        case 4:
            return 0.673;
        case 5:
            return 0.697;
        case 6:
            return 0.709;
        default:
            return 0.7213 / (1 + 1.079 / NUM_REG(precision));
    }
}

/*
 * Computes the raw cardinality estimate
 */
static double raw_estimate(hll_t *h, int *num_zero) {
    unsigned char precision = h->precision;
    int num_reg = NUM_REG(precision);
    double multi = alpha(precision) * num_reg * num_reg;

    int reg_val;
    double inv_sum = 0;
    for (int i=0; i < num_reg; i++) {
        reg_val = get_register(h, i);
        inv_sum += pow(2.0, -1 * reg_val);
        if (!reg_val) *num_zero += 1;
    }
    return multi * (1.0 / inv_sum);
}

/*
 * Estimates cardinality using a linear counting.
 * Used when some registers still have a zero value.
 */
static double linear_count(hll_t *h, int num_zero) {
    int registers = NUM_REG(h->precision);
    return registers *
        log((double)registers / (double)num_zero);
}

/**
 * Binary searches for the nearest matching index
 * @return The matching index, or closest match
 */
static int binary_search(double val, int num, const double *array) {
    int low=0, mid, high=num-1;
    while (low < high) {
        mid = (low + high) / 2;
        if (val > array[mid]) {
            low = mid + 1;
        } else if (val == array[mid]) {
            return mid;
        } else {
            high = mid - 1;
        }
    }
    return low;
}

/**
 * Interpolates the bias estimate using the
 * empircal data collected by Google, from the
 * paper mentioned above.
 */
static double bias_estimate(hll_t *h, double raw_est) {
    // Determine the samples available
    int samples;
    int precision = h->precision;
    switch (precision) {
        case 4:
            samples = 80;
            break;
        case 5:
            samples = 160;
            break;
        default:
            samples = 200;
            break;
    }

    // Get the proper arrays based on precision
    double *estimates = *(rawEstimateData+(precision-4));
    double *biases = *(biasData+(precision-4));

    // Get the matching biases
    int idx = binary_search(raw_est, samples, estimates);
    if (idx == 0)
        return biases[0];
    else if (idx == samples)
        return biases[samples-1];
    else
        return (biases[idx] + biases[idx-1]) / 2;
}

/**
 * Estimates the cardinality of the HLL
 * @arg h The hll to query
 * @return An estimate of the cardinality
 */
double hll_size(hll_t *h) {
    int num_zero = 0;
    double raw_est = raw_estimate(h, &num_zero);

    // Check if we need to apply bias correction
    int num_reg = NUM_REG(h->precision);
    if (raw_est <= 5 * num_reg) {
        raw_est -= bias_estimate(h, raw_est);
    }

    // Check if linear counting should be used
    double alt_est;
    if (num_zero) {
        alt_est = linear_count(h, num_zero);
    } else {
        alt_est = raw_est;
    }

    // Determine which estimate to use
    if (alt_est <= switchThreshold[h->precision-4]) {
        return alt_est;
    } else {
        return raw_est;
    }
}


/**
 * Computes the minimum number of registers
 * needed to hit a target error.
 * @arg error The target error rate
 * @return The number of registers needed, or
 * negative on error.
 */
int hll_precision_for_error(double err) {
    // Check that the error bound is sane
    if (err >= 1 || err <= 0)
        return -1;

    /*
     * Error of HLL is 1.04 / sqrt(m)
     * m is given by 2^p, so solve for p,
     * and use the ceiling.
     */
    double p = log2(pow(1.04 / err, 2));
    return ceil(p);
}

