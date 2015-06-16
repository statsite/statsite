#ifndef _SINK_H_
#define _SINK_H_

#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "metrics.h"

/**
 * Define a base sink type. This can be extended by struct composition
 * if desired.
 */
typedef struct sink {
    const statsite_config* global_config; /**< A pointer to the global
                                           * configuration in use */
    const sink_config* sink_config;       /**< A pointer to the
                                           * configuration used for
                                           * *this* sink. */
    struct sink* next;                    /**< Intrusive linked-list
                                           * for traversing all known
                                           * sinks. */

    /**
     * Invoke this sink for a given set of metrics with optional user
     * data. Current *data is cast to struct timeval for historical
     * reasons until the API gets a good washing.
     *
     * Always pass this instance of sink to this function.
     */
    int (*command)(struct sink*, metrics* m, void* data);

    /**
     * A command to close this sink, allowing any flushing or cleanup
     * actions appropiate for the sink.
     */
    void (*close)(struct sink*);
} sink;

/**
 * Build a set of sinks from the configuration given. Walks
 * sink_configs and builds instances of sink as appropiate.
 */
extern int init_sinks(sink** sinks, statsite_config* config);

#endif
