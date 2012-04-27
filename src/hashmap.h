#ifndef HASHMAP_H
#define HASHMAP_H

/**
 * Opaque hashmap reference
 */
typedef struct hashmap hashmap;
typedef int(*hashmap_callback)(void *data, const char *key, void *value);

/**
 * Creates a new hashmap and allocates space for it.
 * @arg initial_size The minimim initial size. 0 for default (64).
 * @arg map Output. Set to the address of the map
 * @return 0 on success.
 */
int hashmap_init(int initial_size, hashmap **map);

/**
 * Destroys a map and cleans up all associated memory
 * @arg map The hashmap to destroy. Frees memory.
 */
int hashmap_destroy(hashmap *map);

/**
 * Returns the size of the hashmap in items
 */
int hashmap_size(hashmap *map);

/**
 * Gets a value.
 * @arg key The key to look for. Must be null terminated.
 * @arg value Output. Set to the value of th key.
 * 0 on success. -1 if not found.
 */
int hashmap_get(hashmap *map, char *key, void **value);

/**
 * Puts a key/value pair.
 * @arg key The key to set. This is copied, and a seperate
 * version is owned by the hashmap. The caller the key at will.
 * @notes This method is not thread safe.
 * @arg key_len The key length
 * @arg value The value to set.
 * 0 if updated, 1 if added.
 */
int hashmap_put(hashmap *map, char *key, void *value);

/**
 * Deletes a key/value pair.
 * @notes This method is not thread safe.
 * @arg key The key to delete
 * @arg key_len The key length
 * 0 on success. -1 if not found.
 */
int hashmap_delete(hashmap *map, char *key);

/**
 * Clears all the key/value pairs.
 * @notes This method is not thread safe.
 * 0 on success. -1 if not found.
 */
int hashmap_clear(hashmap *map);

/**
 * Iterates through the key/value pairs in the map,
 * invoking a callback for each. The call back gets a
 * key, value for each and returns an integer stop value.
 * If the callback returns 1, then the iteration stops.
 * @arg map The hashmap to iterate over
 * @arg cb The callback function to invoke
 * @arg data Opaque handle passed to the callback
 * @return 0 on success, or the return of the callback.
 */
int hashmap_iter(hashmap *map, hashmap_callback cb, void *data);

#endif
