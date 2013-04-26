#include <stdint.h>
#include "hll.h"

#ifndef SET_H
#define SET_H

/**
 * This is the maximum number of items
 * we represent exactly before switching
 * to a HyperLogLog
 */
#define SET_MAX_EXACT 64

typedef enum {
    EXACT,      // Exact representation, used for small cardinalities
    APPROX      // Approximate representation, used for large cardinalities
} set_type;

typedef struct {
    unsigned char precision;
    uint32_t count;
    uint64_t *hashes;
} exact_set;

typedef struct {
    set_type type;
    union {
        hll_t h;
        exact_set s;
    } store;
} set_t;

/**
 * Initializes a new set
 * @arg precision The precision to use when converting to an HLL
 * @arg s The set to initialize
 * @return 0 on success.
 */
int set_init(unsigned char precision, set_t *s);

/**
 * Destroys the set
 * @return 0 on sucess
 */
int set_destroy(set_t *s);

/**
 * Adds a new key to the set
 * @arg s The set to add to
 * @arg key The key to add
 */
void set_add(set_t *s, char *key);

/**
 * Returns the size of the set. May be approximate.
 * @arg s The set to query
 * @return The size of the set.
 */
uint64_t set_size(set_t *s);


#endif
