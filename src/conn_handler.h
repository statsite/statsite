#ifndef CONN_HANDLER_H
#define CONN_HANDLER_H
#include "config.h"
#include "networking.h"
#include "hashmap.h"
#include "metrics.h"

/**
 * This structure is used to communicate
 * between the connection handlers and the
 * networking layer.
 */
typedef struct {
    statsite_config *config;     // Global configuration
    statsite_conn_info *conn;    // Opaque handle into the networking stack
} statsite_conn_handler;

/** Bucket Support **/
hashmap *METRICS_BUCKETS;
typedef struct {
    time_t *time;
    metrics *metrics;
    time_t *creation_time;
    hashmap *clients;
    int for_global;
} metrics_bucket;

/** Print bucket stats */
int bucket_stats_cb(void *data, const char *key, void *value);

/** Flush respective bucket */
int flush_bucket_cb(void *data, const char *key, void *value);
int bucket_clients_cb(void *data, const char *key, void *value);

void free_metrics_bucket();
metrics_bucket* create_metrics_bucket(int);

/**
 * Invoked to initialize the conn handler layer.
 */
void init_conn_handler(statsite_config *config);

/**
 * Invoked to when we've reached the flush interval timeout
 */
void flush_interval_trigger();

/**
 * Called when statsite is terminating to flush the
 * final set of metrics
 */
void final_flush();

/**
 * Invoked by the networking layer when there is new
 * data to be handled. The connection handler should
 * consume all the input possible, and generate responses
 * to all requests.
 * @arg handle The connection related information
 * @return 0 on success.
 */
int handle_client_connect(statsite_conn_handler *handle);

#endif
