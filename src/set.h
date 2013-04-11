/*
 * Author: Kuba Podgorski
 *
 * Header for the Set functions and data definitions
 */

#ifndef SET_H
#define SET_H

 // Structure for a single set entry (BST node)
typedef struct set_entry {
	// value for this entry
    double value;
    
    // left/less BST node
    struct set_entry* le;

    // right/greater BST node
    struct set_entry* gt;
} set_entry;


// Main struct for representing the set of unique entries
typedef struct set {
    // epsilon for comparing entries' value
    double eps;

	// the number of entries (elements) in the set 
    int active_entries;
    
    // pointer to the BST (Binary Search Tree)
    set_entry* entry;
} set;

// Functions

void set_init(set *, double);

int set_size(set *);

int set_add(set *, double);

int set_foreach(set *, int (*func)(double, void **), void **);

void set_destroy(set *);

#endif