#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <math.h>

#include "binary_types.h"
#include "likely.h"
#include "metrics.h"
#include "sink.h"
#include "streaming.h"
#include "conn_handler.h"

/* Static method declarations */
static int handle_binary_client_connect(statsite_conn_handler *handle);
static int handle_ascii_client_connect(statsite_conn_handler *handle);
static int buffer_after_terminator(char *buf, int buf_len, char terminator, char **after_term, int *after_len);

// This is the magic byte that indicates we are handling
// a binary command, instead of an ASCII command. We use
// 170 (0xaa) value.
static const unsigned char BINARY_MAGIC_BYTE = 0xaa;
static const int MAX_BINARY_HEADER_SIZE = 12;
static const int MIN_BINARY_HEADER_SIZE = 6;

/**
 * This is the current metrics object we are using
 */
static metrics *GLOBAL_METRICS;
static statsite_config *GLOBAL_CONFIG;

/**
 * Invoked to initialize the conn handler layer.
 */
void init_conn_handler(statsite_config *config) {
    // Make the initial metrics object
    metrics *m = malloc(sizeof(metrics));
    int res = init_metrics(config->timer_eps, config->quantiles,
            config->num_quantiles, config->histograms, config->set_precision, m);
    assert(res == 0);
    GLOBAL_METRICS = m;

    // Store the config
    GLOBAL_CONFIG = config;
}

/**
 * A struct passed to the flush thread which contains an instance of
 * metrics and any currently configured sinks.
 */
struct flush_op {
    metrics* m;
    sink* sinks;
};

/**
 * This is the thread that is invoked to handle flushing metrics
 */
static void* flush_thread(void *arg) {
    // Cast the args
    struct flush_op* ops = arg;
    metrics *m = ops->m;
    sink* sinks = ops->sinks;

    // Get the current time
    struct timeval tv;
    gettimeofday(&tv, NULL);

    for (sink* s = sinks; s != NULL; s = s->next) {
        int res = s->command(s, m, &tv);
        if (res != 0) {
            syslog(LOG_WARNING, "Streaming command %s exited with status %d", s->sink_config->name, res);
        }
    }

    // Cleanup
    destroy_metrics(m);
    free(m);
    free(ops);
    return NULL;
}

/**
 * Invoked to when we've reached the flush interval timeout
 */
void flush_interval_trigger(sink* sinks) {
    // Make a new metrics object
    metrics *m = malloc(sizeof(metrics));
    init_metrics(GLOBAL_CONFIG->timer_eps, GLOBAL_CONFIG->quantiles,
            GLOBAL_CONFIG->num_quantiles, GLOBAL_CONFIG->histograms,
            GLOBAL_CONFIG->set_precision, m);

    // Swap with the new one
    struct flush_op* ops = calloc(1, sizeof(struct flush_op));
    ops->m = GLOBAL_METRICS;
    ops->sinks = sinks;

    GLOBAL_METRICS = m;

    // Start a flush thread
    pthread_t thread;
    sigset_t oldset;
    sigset_t newset;
    sigfillset(&newset);
    pthread_sigmask(SIG_BLOCK, &newset, &oldset);
    int err = pthread_create(&thread, NULL, flush_thread, ops);
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);

    if (err == 0) {
        pthread_detach(thread);
        return;
    }

    syslog(LOG_WARNING, "Failed to spawn flush thread: %s", strerror(err));
    GLOBAL_METRICS = ops->m;
    destroy_metrics(m);
    free(m);
}

/**
 * Called when statsite is terminating to flush the
 * final set of metrics
 */
void final_flush(sink* sinks) {
    // Get the last set of metrics
    metrics *old = GLOBAL_METRICS;
    GLOBAL_METRICS = NULL;

    /* We heap allocate this in order to allow it to be freed by the function */
    struct flush_op* ops = calloc(1, sizeof(struct flush_op));
    ops->m = old;
    ops->sinks = sinks;

    flush_thread(ops);
}


/**
 * Invoked by the networking layer when there is new
 * data to be handled. The connection handler should
 * consume all the input possible, and generate responses
 * to all requests.
 * @arg handle The connection related information
 * @return 0 on success.
 */
int handle_client_connect(statsite_conn_handler *handle) {
    // Try to read the magic character, bail if no data
    unsigned char magic;
    if (unlikely(peek_client_byte(handle->conn, &magic) == -1)) return 0;

    // Check the magic byte
    if (magic == BINARY_MAGIC_BYTE)
        return handle_binary_client_connect(handle);
    else
        return handle_ascii_client_connect(handle);
}

/**
 * Invoked to handle ASCII commands. This is the default
 * mode for statsite, to be backwards compatible with statsd
 * @arg handle The connection related information
 * @return 0 on success.
 */
static int handle_ascii_client_connect(statsite_conn_handler *handle) {
    // Look for the next command line
    char *buf, *key, *val_str, *type_str, *sample_str, *endptr;
    metric_type type;
    int buf_len, should_free, status, i, after_len;
    double val, sample_rate;
    while (1) {
        status = extract_to_terminator(handle->conn, '\n', &buf, &buf_len, &should_free);
        if (status == -1) return 0; // Return if no command is available

        // Check for a valid metric
        // Scan for the colon
        status = buffer_after_terminator(buf, buf_len, ':', &val_str, &after_len);
        if (likely(!status)) status |= buffer_after_terminator(val_str, after_len, '|', &type_str, &after_len);
        if (unlikely(status)) {
            syslog(LOG_WARNING, "Failed parse metric! Input: %s", buf);
            goto ERR_RET;
        }

        // Convert the type
        switch (*type_str) {
            case 'c':
                type = COUNTER;
                break;
            case 'h':
            case 'm':
                type = TIMER;
                break;
            case 'k':
                type = KEY_VAL;
                break;
            case 'g':
                type = GAUGE;

                // Check if this is a delta update
                switch (*val_str) {
                    case '+':
                    case '-':
                        type = GAUGE_DELTA;
                }
                break;
            case 's':
                type = SET;
                break;
            default:
                type = UNKNOWN;
                syslog(LOG_WARNING, "Received unknown metric type! Input: %c", *type_str);
                goto ERR_RET;
        }

        // Increment the number of inputs received
        if (GLOBAL_CONFIG->input_counter)
            metrics_add_sample(GLOBAL_METRICS, COUNTER, GLOBAL_CONFIG->input_counter, 1);

        // Fast track the set-updates
        if (type == SET) {
            metrics_set_update(GLOBAL_METRICS, buf, val_str);
            goto END_LOOP;
        }

        // Convert the value to a double
        val = strtod(val_str, &endptr);
        if (unlikely(endptr == val_str)) {
            syslog(LOG_WARNING, "Failed value conversion! Input: %s", val_str);
            goto ERR_RET;
        }

        // Handle counter sampling if applicable
        if (type == COUNTER && !buffer_after_terminator(type_str, after_len, '@', &sample_str, &after_len)) {
            sample_rate = strtod(sample_str, &endptr);
            if (unlikely(endptr == sample_str)) {
                syslog(LOG_WARNING, "Failed sample rate conversion! Input: %s", sample_str);
                goto ERR_RET;
            }
            if (sample_rate > 0 && sample_rate <= 1) {
                // Magnify the value
                val = val * (1.0 / sample_rate);
            }
        }

        // Store the sample
        metrics_add_sample(GLOBAL_METRICS, type, buf, val);

END_LOOP:
        // Make sure to free the command buffer if we need to
        if (should_free) free(buf);
    }

    return 0;
ERR_RET:
    if (should_free) free(buf);
    return -1;
}

// Handles the binary set command
// Return 0 on success, -1 on error, -2 if missing data
static int handle_binary_set(statsite_conn_handler *handle, uint16_t *header, int should_free) {
    /*
     * Abort if we haven't received the command
     * header[1] is the key length
     * header[2] is the set length
     */
    char *key;
    int val_bytes = header[1] + header[2];

    // Read the full command if available
    if (unlikely(should_free)) free(header);
    if (read_client_bytes(handle->conn, MIN_BINARY_HEADER_SIZE + val_bytes, (char**)&header, &should_free))
        return -2;
    key = ((char*)header) + MIN_BINARY_HEADER_SIZE;

    // Verify the null terminators
    if (unlikely(*(key + header[1] - 1))) {
        syslog(LOG_WARNING, "Received command from binary stream with non-null terminated key: %.*s!", header[1], key);
        goto ERR_RET;
    }
    if (unlikely(*(key + val_bytes - 1))) {
        syslog(LOG_WARNING, "Received command from binary stream with non-null terminated set key: %.*s!", header[2], key+header[1]);
        goto ERR_RET;
    }

    // Increment the input counter
    if (GLOBAL_CONFIG->input_counter)
        metrics_add_sample(GLOBAL_METRICS, COUNTER, GLOBAL_CONFIG->input_counter, 1);

    // Update the set
    metrics_set_update(GLOBAL_METRICS, key, key+header[1]);

    // Make sure to free the command buffer if we need to
    if (unlikely(should_free)) free(header);
    return 0;

ERR_RET:
    if (unlikely(should_free)) free(header);
    return -1;
}

/**
 * Invoked to handle binary commands.
 * @arg handle The connection related information
 * @return 0 on success.
 */
static int handle_binary_client_connect(statsite_conn_handler *handle) {
    metric_type type;
    uint16_t key_len;
    int should_free;
    unsigned char *cmd, *key;
    while (1) {
        // Peek and check for the header. This is up to 12 bytes.
        // Magic byte - 1 byte
        // Metric type - 1 byte
        // Key length - 2 bytes
        // Metric value - 8 bytes OR Set Length 2 bytes
        if (peek_client_bytes(handle->conn, MIN_BINARY_HEADER_SIZE, (char**)&cmd, &should_free))
            return 0;  // Return if no command is available

        // Check for the magic byte
        if (unlikely(cmd[0] != BINARY_MAGIC_BYTE)) {
            syslog(LOG_WARNING, "Received command from binary stream without magic byte! Byte: %u", cmd[0]);
            goto ERR_RET;
        }

        // Get the metric type
        switch (cmd[1]) {
            case BIN_TYPE_KV:
                type = KEY_VAL;
                break;
            case BIN_TYPE_COUNTER:
                type = COUNTER;
                break;
            case BIN_TYPE_TIMER:
                type = TIMER;
                break;
            case BIN_TYPE_GAUGE:
                type = GAUGE;
                break;
            case BIN_TYPE_GAUGE_DELTA:
                type = GAUGE_DELTA;
                break;

            // Special case set handling
            case BIN_TYPE_SET:
                switch (handle_binary_set(handle, (uint16_t*)cmd, should_free)) {
                    case -1:
                        return -1;
                    case -2:
                        return 0;
                    default:
                        continue;
                }

            default:
                syslog(LOG_WARNING, "Received command from binary stream with unknown type: %u!", cmd[1]);
                goto ERR_RET;
        }

        // Abort if we haven't received the full key, wait for the data
        key_len = *(uint16_t*)(cmd+2);

        // Read the full command if available
        if (unlikely(should_free)) free(cmd);
        if (read_client_bytes(handle->conn, MAX_BINARY_HEADER_SIZE + key_len, (char**)&cmd, &should_free))
            return 0;
        key = cmd + MAX_BINARY_HEADER_SIZE;

        // Verify the key contains a null terminator
        if (unlikely(*(key + key_len - 1))) {
            syslog(LOG_WARNING, "Received command from binary stream with non-null terminated key: %.*s!", key_len, key);
            goto ERR_RET;
        }

        // Increment the input counter
        if (GLOBAL_CONFIG->input_counter)
            metrics_add_sample(GLOBAL_METRICS, COUNTER, GLOBAL_CONFIG->input_counter, 1);

        // Add the sample
        metrics_add_sample(GLOBAL_METRICS, type, (char*)key, *(double*)(cmd+4));

        // Make sure to free the command buffer if we need to
        if (unlikely(should_free)) free(cmd);
    }
    return 0;
ERR_RET:
    if (unlikely(should_free)) free(cmd);
    return -1;
}


/**
 * Scans the input buffer of a given length up to a terminator.
 * Then sets the start of the buffer after the terminator including
 * the length of the after buffer.
 * @arg buf The input buffer
 * @arg buf_len The length of the input buffer
 * @arg terminator The terminator to scan to. Replaced with the null terminator.
 * @arg after_term Output. Set to the byte after the terminator.
 * @arg after_len Output. Set to the length of the output buffer.
 * @return 0 if terminator found. -1 otherwise.
 */
static int buffer_after_terminator(char *buf, int buf_len, char terminator, char **after_term, int *after_len) {
    // Scan for a space
    char *term_addr = memchr(buf, terminator, buf_len);
    if (!term_addr) {
        *after_term = NULL;
        return -1;
    }

    // Convert the space to a null-seperator
    *term_addr = '\0';

    // Provide the arg buffer, and arg_len
    *after_term = term_addr+1;
    *after_len = buf_len - (term_addr - buf + 1);
    return 0;
}
