#include <stdint.h>

#ifndef HLL_H
#define HLL_H

// Ensure precision in a sane bound
#define HLL_MIN_PRECISION 4      // 16 registers
#define HLL_MAX_PRECISION 18     // 262,144 registers

typedef struct {
    unsigned char precision;
    uint32_t *registers;
} hll_t;

/**
 * Initializes a new HLL
 * @arg precision The digits of precision to use
 * @arg h The HLL to initialize
 * @return 0 on success
 */
int hll_init(unsigned char precision, hll_t *h);

/**
 * Destroys an hll
 * @return 0 on success
 */
int hll_destroy(hll_t *h);

/**
 * Adds a new key to the HLL
 * @arg h The hll to add to
 * @arg key The key to add
 */
void hll_add(hll_t *h, char *key);

/**
 * Adds a new hash to the HLL
 * @arg h The hll to add to
 * @arg hash The hash to add
 */
void hll_add_hash(hll_t *h, uint64_t hash);

/**
 * Estimates the cardinality of the HLL
 * @arg h The hll to query
 * @return An estimate of the cardinality
 */
double hll_size(hll_t *h);

/**
 * Computes the minimum digits of precision
 * needed to hit a target error.
 * @arg error The target error rate
 * @return The number of digits needed, or
 * negative on error.
 */
int hll_precision_for_error(double err);

#endif
