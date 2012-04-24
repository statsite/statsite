/*
 * Author: Armon Dadgar
 *
 * Header for the Heap functions and data definitions
 */

#ifndef HEAP_H
#define HEAP_H

// Structure for a single heap entry
typedef struct heap_entry {
    void* key;   // Key for this entry
    void* value; // Value for this entry
} heap_entry;


// Main struct for representing the heap
typedef struct heap {
    int (*compare_func)(void*, void*); // The key comparison function to use
    int active_entries;  // The number of entries in the heap
    int minimum_pages;   // The minimum number of pages to maintain, based on the initial cap.
    int allocated_pages; // The number of pages in memory that are allocated
    heap_entry* table; // Pointer to the table, which maps to the pages
} heap;

// Functions

/**
 * Creates a new heap
 * @param h Pointer to a heap structure that is initialized
 * @param initial_size What should the initial size of the heap be. If <= 0, then it will be set to the minimum
 * permissable size, of 1 page (512 entries on 32bit system with 4K pages).
 * @param comp_func A pointer to a function that can be used to compare the keys. If NULL, it will be set
 * to a function which treats keys as signed ints. This function must take two keys, given as pointers and return an int.
 * It should return -1 if key 1 is smaller, 0 if they are equal, and 1 if key 2 is smaller.
 */
void heap_create(heap* h, int initial_size, int (*comp_func)(void*,void*));

/**
 * Returns the size of the heap
 * @param h Pointer to a heap structure
 * @return The number of entries in the heap.
 */
int heap_size(heap* h);

/**
 * Inserts a new element into a heap.
 * @param h The heap to insert into
 * @param key The key of the new entry
 * @param value The value of the new entry
 */
void heap_insert(heap* h, void* key, void* value);

/**
 * Returns the element with the smallest key in the heap.
 * @param h Pointer to the heap structure
 * @param key A pointer to a pointer, to set to the minimum key
 * @param value Set to the value corresponding with the key
 * @return 1 if the minimum element exists and is set, 0 if there are no elements.
 */
int heap_min(heap* h, void** key, void** value);

/**
 * Deletes the element with the smallest key from the heap.
 * @param h Pointer to the heap structure
 * @param key A pointer to a pointer, to set to the minimum key
 * @param valu Set to the value corresponding with the key
 * @return 1if the minimum element exists and is deleted, 0 if there are no elements.
 */
int heap_delmin(heap* h, void** key, void** value);

/**
 * Calls a function for each entry in the heap.
 * @param h The heap to iterate over
 * @param func The function to call on each entry. Should take a void* key and value.
 */
void heap_foreach(heap* h, void (*func)(void*,void*));

/**
 * Destroys and cleans up a heap.
 * @param h The heap to destroy.
 */
void heap_destroy(heap* h);

#endif

