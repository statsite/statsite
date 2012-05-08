#ifndef CONN_HANDLER_H
#define CONN_HANDLER_H
#include "config.h"
#include "networking.h"

/**
 * This structure is used to communicate
 * between the connection handlers and the
 * networking layer.
 */
typedef struct {
    statsite_config *config;     // Global configuration
    statsite_conn_info *conn;    // Opaque handle into the networking stack
} statsite_conn_handler;

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
