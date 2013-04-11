/**
 * This file defines the methods declared in set.h
 * These are used to create and manipulate a set
 * data structure.
 */

#include <stdlib.h>
#include <syslog.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include "set.h"

// Helpers definition
static set_entry* set_entry_add(set_entry *, int *, char *);
static int set_entry_foreach(set_entry *, int (*func)(char *, void **), void **);
static set_entry* set_entry_destroy(set_entry *, int *);
// 

// Header's implementation
void set_init(set *s) {
	    assert(s != NULL);

	    s -> active_entries = 0;
	    s -> entry = NULL;
}

int set_size(set *s) {
	assert(s != NULL);
	return s -> active_entries;
}

int set_add(set *s, char *value) {
	assert(s != NULL);

	int is_ok = 0, n = s -> active_entries;
	s -> entry = set_entry_add(s -> entry, &s->active_entries, value);
	is_ok = s -> active_entries - n;

	syslog(LOG_DEBUG, "\tset_add(%s)\t [%s]\tactive_entries: %d\n", value, is_ok ? "+": "=", s -> active_entries);
	return 0;
}

int set_foreach(set* s, int (*func_cb)(char *, void **), void **argv) {
	assert(s != NULL);
	assert(func_cb != NULL);

	return set_entry_foreach(s -> entry, func_cb, argv);
}

void set_destroy(set *s) {
	assert(s != NULL);
	s -> entry = set_entry_destroy(s -> entry, &s -> active_entries);
}
//

// Helpers implementation
static set_entry * set_entry_add(set_entry* entry, int *active_entries, char *value) {	
	if (entry == NULL) {		
		entry = (set_entry *)malloc(sizeof(set_entry));
		if (!entry) return 0;

		entry -> le = NULL;
		entry -> gt = NULL;
		entry -> value = strdup(value);

		++(*active_entries);
	} 
	else {
		int cmp = strcmp(value, entry -> value);
		if (cmp < 0) entry -> le = set_entry_add(entry -> le, active_entries, value);
		else if (cmp > 0) entry -> gt =  set_entry_add(entry -> gt, active_entries, value);		
	}

	return entry;	
}

static int set_entry_foreach(set_entry* entry, int (*func_cb)(char *, void **), void **argv) {
	if (entry != NULL) {
		if (func_cb(entry -> value, argv)) return -1;
		if (set_entry_foreach(entry -> le, func_cb, argv)) return -1;
		if (set_entry_foreach(entry -> gt, func_cb, argv)) return -1;
	}

	return 0;
}

static set_entry * set_entry_destroy(set_entry* entry, int *active_entries) {
	if (entry == NULL) return entry;

	entry -> le = set_entry_destroy(entry -> le, active_entries);
	entry -> gt = set_entry_destroy(entry -> gt, active_entries);

	if (NULL == entry -> le && NULL == entry -> gt) {
		free(entry -> value);
		free(entry);
		entry = NULL;		
		--(*active_entries);
	}
	return entry;
}