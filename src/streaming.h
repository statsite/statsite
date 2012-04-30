#ifndef STREAMING_H
#define STREAMING_H
#include <stdio.h>
#include "metrics.h"

/**
 * This callback is used to stream data to the external command.
 * It is provided with all the metrics and a pipe. The command should
 * return 1 to terminate. See metric_callback for more info.
 */
typedef int(*stream_callback)(FILE *pipe, void *data, metric_type type, char *name, void *value);

/**
 * Streams the metrics stored in a metrics object to an external command
 * @arg m The metrics object to stream
 * @arg data An opaque handle passed to the callback
 * @arg cb The callback to invoke
 * @arg cmd The command to invoke, invoked with a shell.
 * @return 0 on success, or the value of stream callback.
 */
int stream_to_command(metrics *m, void *data, stream_callback cb, char *cmd);

#endif

