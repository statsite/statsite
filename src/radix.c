#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include "radix.h"

/**
 * Initializes the radix tree
 * @arg tree The tree to initialize
 * @return 0 on success
 */
int radix_init(radix_tree *tree) {
    bzero(tree, sizeof(radix_tree));
    return 0;
}

// Recursively destroys the radix tree
static void recursive_destroy(radix_node *n) {
    radix_node *child;
    if (n->edges[0]) {
        radix_leaf *leaf = n->edges[0];
        free(leaf->key);
        free(leaf);
    }
    for (int i=1; i < 256; i++) {
        child = (radix_node*)n->edges[i];
        if (!child) continue;
        recursive_destroy(child);
        free(child);
    }
}

/**
 * Destroys the tree
 * @arg tree The tree to destroy
 * @return 0 on success
 */
int radix_destroy(radix_tree *tree) {
    recursive_destroy(&tree->root);
    return 0;
}

// Computes the longest prefix of two null terminated strings
static int longest_prefix(char *k1, char *k2, int max) {
    int i;
    for (i=0; i < max; i++) {
        if (!k1[i] || !k2[i] || k1[i] != k2[i])
            break;
    }
    return i;
}

/**
 * Inserts a value into the tree
 * @arg t The tree to insert into
 * @arg key The key of the value
 * @arg value Initially points to the value to insert, replaced
 * by the value that was updated if any
 * @return 0 if the value was inserted, 1 if the value was updated.
 */
int radix_insert(radix_tree *t, char *key, void **value) {
    radix_node *child, *parent=NULL, *n = &t->root;
    radix_leaf *leaf;
    char *search = key;
    int common_prefix;
    do {
        // Check if we've exhausted the key
        if (!search || *search == 0) {
            leaf = n->edges[0];
            if (leaf) {
                // Return the old value
                void *old = leaf->value;
                leaf->value = *value;
                *value = old;
                return 1;
            } else {
                // Add a new node
                leaf = malloc(sizeof(radix_leaf));
                n->edges[0] = leaf;
                leaf->key = key ? strdup(key) : NULL;
                leaf->value = *value;
                return 0;
            }
        }

        // Get the edge
        parent = n;
        n = n->edges[(int)*search];
        if (!n) {
            child = parent->edges[(int)*search] = calloc(1, sizeof(radix_node));
            leaf = child->edges[0] = malloc(sizeof(radix_leaf));
            leaf->key = strdup(key);
            leaf->value = *value;

            // The key of the node is some
            child->key = leaf->key + (search - key);
            child->key_len = strlen(search);
            return 0;
        }

        // Determine longest prefix of the search key on match
        common_prefix = longest_prefix(search, n->key, n->key_len);
        if (common_prefix == n->key_len) {
            search += n->key_len;

        // If we share a sub-set, we need to split the nodes
        } else {
            // Split the node with the shared prefix
            child = calloc(1, sizeof(radix_node));
            parent->edges[(int)*search] = child;
            child->key = n->key;
            child->key_len = common_prefix;

            // Restore pointer to the existing node
            n->key += common_prefix;
            n->key_len -= common_prefix;
            child->edges[(int)*(n->key)] = n;

            // Create a new leaf
            leaf = malloc(sizeof(radix_leaf));
            leaf->key = strdup(key);
            leaf->value = *value;

            // If the new key is a subset, add to to this node
            search += common_prefix;
            if (*search == 0) {
                child->edges[0] = leaf;
                return 0;
            }

            // Create a new node for the new key
            n = calloc(1, sizeof(radix_node));
            child->edges[(int)*search] = n;
            n->edges[0] = leaf;

            // The key of the node is some
            n->key = leaf->key + (search - key);
            n->key_len = strlen(search);
            return 0;
        }
    } while (1);
    return 0;
}

/**
 * Finds a value in the tree
 * @arg t The tree to search
 * @arg key The key to search
 * @arg value The value of the key
 * @return 0 if found
 */
int radix_search(radix_tree *t, char *key, void **value) {
    radix_node *n = &t->root;
    char *search = key;
    do {
        // Check if we've exhausted the key
        if (!search || *search == 0) {
            radix_leaf *l = n->edges[0];
            if (l) {
                *value = l->value;
                return 0;
            }
            break;
        }

        // Get the edge
        n = n->edges[(int)*search];
        if (!n) break;

        // Consume the search key on match
        if (!strncmp(search, n->key, n->key_len))
            search += n->key_len;
        else
            break;
    } while (1);
    return 1;
}

/**
 * Finds the longest matching prefix
 * @arg t The tree to search
 * @arg key The key to search
 * @arg value The value of the key with the longest prefix
 * @return 0 if found
 */
int radix_longest_prefix(radix_tree *t, char *key, void **value) {
    radix_node *n = &t->root;
    radix_leaf *last_match = NULL;
    char *search = key;
    do {
        // Store the last match
        if (n->edges[0])
            last_match = n->edges[0];

        // Check if we've exhausted the key
        if (!search || *search == 0)
            break;

        // Get the edge
        n = n->edges[(int)*search];
        if (!n) break;

        // Consume the search key on match
        if (!strncmp(search, n->key, n->key_len))
            search += n->key_len;
        else
            break;
    } while (1);

    // Check if we found a leaf
    if (last_match) {
        *value = last_match->value;
        return 0;
    }
    return 1;
}

// Recursively iterates
static int recursive_iter(radix_node *n, void *data, int(*iter_func)(void *data, char *key, void *value)) {
    int ret = 0;
    if (n->edges[0]) {
        radix_leaf *leaf = n->edges[0];
        ret = iter_func(data, leaf->key, leaf->value);
    }
    radix_node *child;
    for (int i=1; !ret && i < 256; i++) {
        child = (radix_node*)n->edges[i];
        if (!child) continue;
        ret = recursive_iter(child, data, iter_func);
    }
    return ret;
}

/**
 * Iterates through all the nodes in the radix tree
 * @arg t The tree to iter through
 * @arg iter_func A callback function to iterate through. Returns
 * non-zero to stop iteration.
 * @return 0 on sucess. 1 if the iteration was stopped.
 */
int radix_foreach(radix_tree *t, void *data, int(*iter_func)(void* data, char *key, void *value)) {
    return recursive_iter(&t->root, data, iter_func);
}

