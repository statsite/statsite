/**
 * This file defines the methods declared in set.h
 * These are used to create and manipulate a set
 * data structure.
 */

#include <stdlib.h>
#include <syslog.h>
#include <assert.h>
#include <math.h>
#include "set.h"

// Helpers definition
static set_entry* set_entry_add(set_entry *, int *, double, double);
static int set_entry_foreach(set_entry *, int (*func)(double, void **), void **);
static set_entry* set_entry_destroy(set_entry *, int *);
// 

// Header's implementation
void set_init(set *s, double eps) {
	    assert(s != NULL);

	    s -> eps = eps;
	    s -> active_entries = 0;
	    s -> entry = NULL;
}

int set_size(set *s) {
	assert(s != NULL);
	return s -> active_entries;
}

int set_add(set *s, double value) {
	assert(s != NULL);

	int is_ok = 0, n = s -> active_entries;
	s -> entry = set_entry_add(s -> entry, &s->active_entries, value, s -> eps);
	is_ok = s -> active_entries - n;

	syslog(LOG_DEBUG, "\tset_add(%lf)\t [%s]\tactive_entries: %d\n", value, is_ok ? "+": "=", s -> active_entries);
	return 0;
}

int set_foreach(set* s, int (*func)(double, void **), void **argv) {
	assert(s != NULL);
	assert(func != NULL);

	return set_entry_foreach(s -> entry, func, argv);
}

void set_destroy(set *s) {
	assert(s != NULL);
	s -> entry = set_entry_destroy(s -> entry, &s -> active_entries);
	
	syslog(LOG_DEBUG, "set->active_entries: %d", s -> active_entries);
}
//

// Helpers implementation
static set_entry * set_entry_add(set_entry* entry, int *active_entries, double value, double eps) {	
	if (entry == NULL) {		
		entry = (set_entry *)malloc(sizeof(set_entry));
		if (!entry) return 0;

		entry -> le = NULL;
		entry -> gt = NULL;
		entry -> value = value;

		++(*active_entries);
	} 
	else {
		double d = value - entry -> value; 
		if (fabs(d) > eps) {
			if (d < 0.0) entry -> le = set_entry_add(entry -> le, active_entries, value, eps);
			else if (d > 0.0) entry -> gt =  set_entry_add(entry -> gt, active_entries, value, eps);
		}
	}

	return entry;	
}

static int set_entry_foreach(set_entry* entry, int (*func)(double, void **), void **argv) {
	if (entry != NULL) {
		if (func(entry -> value, argv)) return -1;
		if (set_entry_foreach(entry -> le, func, argv)) return -1;
		if (set_entry_foreach(entry -> gt, func, argv)) return -1;
	}

	return 0;
}

static set_entry * set_entry_destroy(set_entry* entry, int *active_entries) {
	if (entry == NULL) return entry;

	entry -> le = set_entry_destroy(entry -> le, active_entries);
	entry -> gt = set_entry_destroy(entry -> gt, active_entries);

	if (NULL == entry -> le && NULL == entry -> gt) {
		free(entry);
		entry = NULL;		
		--(*active_entries);
	}
	return entry;
}