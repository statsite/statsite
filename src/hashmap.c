#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "hashmap.h"

#define MAX_CAPACITY 0.75
#define DEFAULT_CAPACITY 128

// Basic hash entry.
typedef struct hashmap_entry {
    char *key;
    void *value;
    struct hashmap_entry *next; // Support linking.
} hashmap_entry;

struct hashmap {
    int count;      // Number of entries
    int table_size; // Size of table in nodes
    int max_size;   // Max size before we resize
    hashmap_entry *table; // Pointer to an arry of hashmap_entry objects
};

// Link the external murmur hash in
extern void MurmurHash3_x64_128(const void * key, const int len, const uint32_t seed, void *out);

/**
 * Creates a new hashmap and allocates space for it.
 * @arg initial_size The minimim initial size. 0 for default (64).
 * @arg map Output. Set to the address of the map
 * @return 0 on success.
 */
int hashmap_init(int initial_size, hashmap **map) {
    // Default to 64 if no size
    if (initial_size <= 0) {
       initial_size = DEFAULT_CAPACITY;

    // Round up to power of 2
    } else {
        int most_sig_bit = 0;
        for (int idx= 0; idx < sizeof(initial_size)*8; idx++) {
            if ((initial_size >> idx) & 0x1)
                most_sig_bit = idx;
        }

        // Round up if not a power of 2
        if ((1 << most_sig_bit) != initial_size) {
            most_sig_bit += 1;
        }
        initial_size = 1 << most_sig_bit;
    }

    // Allocate the map
    hashmap *m = calloc(1, sizeof(hashmap));
    m->table_size = initial_size;
    m->max_size = MAX_CAPACITY * initial_size;

    // Allocate the table
    m->table = (hashmap_entry*)calloc(initial_size, sizeof(hashmap_entry));

    // Return the table
    *map = m;
    return 0;
}

/**
 * Destroys a map and cleans up all associated memory
 * @arg map The hashmap to destroy. Frees memory.
 */
int hashmap_destroy(hashmap *map) {
    // Free each entry
    hashmap_entry *entry, *old;
    int in_table;
    for (int i=0; i < map->table_size; i++) {
        entry = map->table+i;
        in_table = 1;
        while (entry && entry->key) {
            // Walk the next links
            old = entry;
            entry = entry->next;

            // Clear the objects
            free(old->key);

            // The initial entry is in the table
            // and we should not free that one.
            if (!in_table) {
                free(old);
            }
            in_table = 0;
        }
    }

    // Free the table and hash map
    free(map->table);
    free(map);
    return 0;
}

/**
 * Returns the size of the hashmap in items
 */
int hashmap_size(hashmap *map) {
    return map->count;
}

/**
 * Gets a value.
 * @arg key The key to look for
 * @arg key_len The key length
 * @arg value Output. Set to the value of th key.
 * 0 on success. -1 if not found.
 */
int hashmap_get(hashmap *map, char *key, void **value) {
    // Compute the hash value of the key
    uint64_t out[2];
    MurmurHash3_x64_128(key, strlen(key), 0, &out);

    // Mod the lower 64bits of the hash function with the table
    // size to get the index
    unsigned int index = out[1] % map->table_size;

    // Look for an entry
    hashmap_entry *entry = map->table+index;

    // Scan the keys
    while (entry && entry->key) {
        // Found it
        if (strcmp(entry->key, key) == 0) {
            *value = entry->value;
            return 0;
        }

        // Walk the chain
        entry = entry->next;
    }

    // Not found
    return -1;
}

/**
 * Internal method to insert into a hash table
 * @arg table The table to insert into
 * @arg table_size The size of the table
 * @arg key The key to insert
 * @arg key_len The length of the key
 * @arg value The value to associate
 * @arg should_cmp Should keys be compared to existing ones.
 * @arg should_dup Should duplicate keys
 * @return 1 if the key is new, 0 if updated.
 */
static int hashmap_insert_table(hashmap_entry *table, int table_size, char *key, int key_len,
                                void *value, int should_cmp, int should_dup) {
    // Compute the hash value of the key
    uint64_t out[2];
    MurmurHash3_x64_128(key, key_len, 0, &out);

    // Mod the lower 64bits of the hash function with the table
    // size to get the index
    unsigned int index = out[1] % table_size;

    // Look for an entry
    hashmap_entry *entry = table+index;
    hashmap_entry *last_entry = NULL;

    // Scan the keys
    while (entry && entry->key) {
        // Found it, update the value
        if (should_cmp && (strcmp(entry->key, key) == 0)) {
            entry->value = value;
            return 0;
        }

        // Walk the chain
        last_entry = entry;
        entry = entry->next;
    }

    // If last entry is NULL, we can just
    // insert directly into the table slot
    // since it is empty
    if (last_entry == NULL) {
        entry->key = (should_dup) ? strdup(key) : key;
        entry->value = value;

    // We have a last value, need to link against it
    } else {
        entry = calloc(1, sizeof(hashmap_entry));
        entry->key = (should_dup) ? strdup(key) : key;
        entry->value = value;
        last_entry->next = entry;
    }
    return 1;
}


/**
 * Internal method to double the size of a hashmap
 */
static void hashmap_double_size(hashmap *map) {
    // Calculate the new sizes
    int new_size = map->table_size * 2;
    int new_max_size = map->max_size * 2;

    // Allocate the table
    hashmap_entry *new_table = (hashmap_entry*)calloc(new_size, sizeof(hashmap_entry));

    // Move each entry
    hashmap_entry *entry, *old;
    int in_table;
    for (int i=0; i < map->table_size; i++) {
        entry = map->table+i;
        in_table = 1;
        while (entry && entry->key) {
            // Walk the next links
            old = entry;
            entry = entry->next;

            // Insert the value in the new map
            // Do not compare keys or duplicate since we are just doubling our
            // size, and we have unique keys and duplicates already.
            hashmap_insert_table(new_table, new_size, old->key, strlen(old->key),
                    old->value, 0, 0);

            // The initial entry is in the table
            // and we should not free that one.
            if (!in_table) {
                free(old);
            }
            in_table = 0;
        }
    }

    // Free the old table
    free(map->table);

    // Update the pointers
    map->table = new_table;
    map->table_size = new_size;
    map->max_size = new_max_size;
}

/**
 * Puts a key/value pair. Replaces existing values.
 * @arg key The key to set. This is copied, and a seperate
 * version is owned by the hashmap. The caller the key at will.
 * @notes This method is not thread safe.
 * @arg key_len The key length
 * @arg value The value to set.
 * 0 if updated, 1 if added.
 */
int hashmap_put(hashmap *map, char *key, void *value) {
    // Check if we need to double the size
    if (map->count + 1 > map->max_size) {
        // Doubles the size of the hashmap, re-try the insert
        hashmap_double_size(map);
        return hashmap_put(map, key, value);
    }

    // Insert into the map, comparing keys and duplicating keys
    int new = hashmap_insert_table(map->table, map->table_size, key, strlen(key), value, 1, 1);
    if (new) map->count += 1;

    return new;
}

/**
 * Deletes a key/value pair.
 * @notes This method is not thread safe.
 * @arg key The key to delete
 * 0 on success. -1 if not found.
 */
int hashmap_delete(hashmap *map, char *key) {
    // Compute the hash value of the key
    uint64_t out[2];
    MurmurHash3_x64_128(key, strlen(key), 0, &out);

    // Mod the lower 64bits of the hash function with the table
    // size to get the index
    unsigned int index = out[1] % map->table_size;

    // Look for an entry
    hashmap_entry *entry = map->table+index;
    hashmap_entry *last_entry = NULL;

    // Scan the keys
    while (entry && entry->key) {
        // Found it
        if (strcmp(entry->key, key) == 0) {
            // Free the key
            free(entry->key);
            map->count -= 1;

            // Check if we are in the table
            if (last_entry == NULL) {
                // Copy the keys from the node, and free it
                if (entry->next) {
                    hashmap_entry *n = entry->next;
                    entry->key = n->key;
                    entry->value = n->value;
                    entry->next = n->next;
                    free(n);

                // Zero everything out
                } else {
                    entry->key = NULL;
                    entry->value = NULL;
                }

            } else {
                // Skip us in the list
                last_entry->next = entry->next;

                // Free ourself
                free(entry);
            }
            return 0;
        }
        // Walk the chain
        entry = entry->next;
    }

    // Not found
    return -1;
}

/**
 * Clears all the key/value pairs.
 * @notes This method is not thread safe.
 * 0 on success. -1 if not found.
 */
int hashmap_clear(hashmap *map) {
    hashmap_entry *entry, *old;
    int in_table;
    for (int i=0; i < map->table_size; i++) {
        entry = map->table+i;
        in_table = 1;
        while (entry && entry->key) {
            // Walk the next links
            old = entry;
            entry = entry->next;

            // Clear the objects
            free(old->key);

            // The initial entry is in the table
            // and we should not free that one.
            if (!in_table) {
                free(old);
            } else {
                old->key = NULL;
            }
            in_table = 0;
        }
    }

    // Reset the sizes
    map->count = 0;
    return 0;
}

/**
 * Iterates through the key/value pairs in the map,
 * invoking a callback for each. The call back gets a
 * key, value for each and returns an integer stop value.
 * If the callback returns 1, then the iteration stops.
 * @arg map The hashmap to iterate over
 * @arg cb The callback function to invoke
 * @arg data Opaque handle passed to the callback
 * @return 0 on success
 */
int hashmap_iter(hashmap *map, hashmap_callback cb, void *data) {
    hashmap_entry *entry;
    int should_break = 0;
    for (int i=0; i < map->table_size && !should_break; i++) {
        entry = map->table+i;
        while (entry && entry->key && !should_break) {
            // Invoke the callback
            should_break = cb(data, entry->key, entry->value);
            entry = entry->next;
        }
    }
    return should_break;
}

