#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <netinet/in.h>
#include "metrics.h"
#include "streaming.h"
#include "conn_handler.h"

/*
 * Binary defines
 */
#define BIN_TYPE_KV      0x1
#define BIN_TYPE_COUNTER 0x2
#define BIN_TYPE_TIMER   0x3

#define BIN_OUT_NO_TYPE 0x0
#define BIN_OUT_SUM     0x1
#define BIN_OUT_SUM_SQ  0x2
#define BIN_OUT_MEAN    0x3
#define BIN_OUT_COUNT   0x4
#define BIN_OUT_STDDEV  0x5
#define BIN_OUT_MIN     0x6
#define BIN_OUT_MAX     0x7
#define BIN_OUT_PCT     0x80

/* Static method declarations */
static int handle_binary_client_connect(statsite_conn_handler *handle);
static int handle_ascii_client_connect(statsite_conn_handler *handle);
static int buffer_after_terminator(char *buf, int buf_len, char terminator, char **after_term, int *after_len);

/* These are the quantiles we track */
static double QUANTILES[] = {0.5, 0.95, 0.99};
static int NUM_QUANTILES = 3;

// This is the magic byte that indicates we are handling
// a binary command, instead of an ASCII command. We use
// 170 (0xaa) value.
static unsigned char BINARY_MAGIC_BYTE = 0xaa;
static int BINARY_HEADER_SIZE = 12;

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
    int res = init_metrics(config->timer_eps, (double*)&QUANTILES, NUM_QUANTILES, m);
    assert(res == 0);
    GLOBAL_METRICS = m;

    // Store the config
    GLOBAL_CONFIG = config;
}

/**
 * Streaming callback to format our output
 */
static int stream_formatter(FILE *pipe, void *data, metric_type type, char *name, void *value) {
    #define STREAM(...) if (fprintf(pipe, __VA_ARGS__, (long long)tv->tv_sec) < 0) return 1;
    struct timeval *tv = data;
    switch (type) {
        case KEY_VAL:
            STREAM("kv.%s|%f|%lld\n", name, *(double*)value);
            break;

        case COUNTER:
            STREAM("counts.%s|%f|%lld\n", name, counter_sum(value));
            break;

        case TIMER:
            STREAM("timers.%s.sum|%f|%lld\n", name, timer_sum(value));
            STREAM("timers.%s.mean|%f|%lld\n", name, timer_mean(value));
            STREAM("timers.%s.lower|%f|%lld\n", name, timer_min(value));
            STREAM("timers.%s.upper|%f|%lld\n", name, timer_max(value));
            STREAM("timers.%s.count|%lld|%lld\n", name, timer_count(value));
            STREAM("timers.%s.stdev|%f|%lld\n", name, timer_stddev(value));
            STREAM("timers.%s.median|%f|%lld\n", name, timer_query(value, 0.5));
            STREAM("timers.%s.p95|%f|%lld\n", name, timer_query(value, 0.95));
            STREAM("timers.%s.p99|%f|%lld\n", name, timer_query(value, 0.99));
            break;

        default:
            syslog(LOG_ERR, "Unknown metric type: %d", type);
            break;
    }
    return 0;
}

/* Helps to write out a single binary result */
#pragma pack(push,1)
struct binary_out_prefix {
    uint64_t timestamp;
    uint8_t  type;
    uint8_t  value_type;
    uint16_t key_len;
    double   val;
};
#pragma pack(pop)

static int stream_bin_writer(FILE *pipe, uint64_t timestamp, unsigned char type,
        unsigned char val_type, double val, char *name) {
        uint16_t key_len = strlen(name) + 1;
        struct binary_out_prefix out = {timestamp, type, val_type, key_len, val};
        if (!fwrite(&out, sizeof(struct binary_out_prefix), 1, pipe)) return 1;
        if (!fwrite(name, key_len, 1, pipe)) return 1;
        return 0;
}

static int stream_formatter_bin(FILE *pipe, void *data, metric_type type, char *name, void *value) {
    #define STREAM_BIN(...) if (stream_bin_writer(pipe, ((struct timeval *)data)->tv_sec, __VA_ARGS__, name)) return 1;
    switch (type) {
        case KEY_VAL:
            STREAM_BIN(BIN_TYPE_KV, BIN_OUT_NO_TYPE, *(double*)value);
            break;

        case COUNTER:
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_SUM, counter_sum(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_SUM_SQ, counter_squared_sum(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_MEAN, counter_mean(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_COUNT, counter_count(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_STDDEV, counter_stddev(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_MIN, counter_min(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_MAX, counter_max(value));
            break;

        case TIMER:
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_SUM, timer_sum(value));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_SUM_SQ, timer_squared_sum(value));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_MEAN, timer_mean(value));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_COUNT, timer_count(value));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_STDDEV, timer_stddev(value));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_MIN, timer_min(value));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_MAX, timer_max(value));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_PCT | 50, timer_query(value, 0.5));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_PCT | 95, timer_query(value, 0.95));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_PCT | 99, timer_query(value, 0.99));
            break;

        default:
            syslog(LOG_ERR, "Unknown metric type: %d", type);
            break;
    }
    return 0;
}

/**
 * This is the thread that is invoked to handle flushing metrics
 */
static void* flush_thread(void *arg) {
    // Cast the args
    metrics *m = arg;

    // Get the current time
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Determine which callback to use
    stream_callback cb = (GLOBAL_CONFIG->binary_stream)? stream_formatter_bin: stream_formatter;

    // Stream the records
    int res = stream_to_command(m, &tv, cb, GLOBAL_CONFIG->stream_cmd);
    if (res != 0) {
        syslog(LOG_WARNING, "Streaming command exited with status %d", res);
    }

    // Cleanup
    destroy_metrics(m);
    free(m);
}

/**
 * Invoked to when we've reached the flush interval timeout
 */
void flush_interval_trigger() {
    // Make a new metrics object
    metrics *m = malloc(sizeof(metrics));
    init_metrics(GLOBAL_METRICS->eps, (double*)&QUANTILES, NUM_QUANTILES, m);

    // Swap with the new one
    metrics *old = GLOBAL_METRICS;
    GLOBAL_METRICS = m;

    // Start a flush thread
    pthread_t thread;
    pthread_create(&thread, NULL, flush_thread, old);
    pthread_detach(thread);
}

/**
 * Called when statsite is terminating to flush the
 * final set of metrics
 */
void final_flush() {
    // Get the last set of metrics
    metrics *old = GLOBAL_METRICS;
    GLOBAL_METRICS = NULL;

    // Start a flush thread
    pthread_t thread;
    pthread_create(&thread, NULL, flush_thread, old);

    // Wait for the thread to finish
    pthread_join(thread, NULL);
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
    unsigned char magic = 0;
    if (peek_client_bytes(handle->conn, 1, &magic) == -1) return 0;

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
    char *buf, *key, *val_str, *type_str, *endptr;
    metric_type type;
    int buf_len, should_free, status, i, after_len;
    double val;
    while (1) {
        status = extract_to_terminator(handle->conn, '\n', &buf, &buf_len, &should_free);
        if (status == -1) return 0; // Return if no command is available

        // Check for a valid metric
        // Scan for the colon
        status = buffer_after_terminator(buf, buf_len, ':', &val_str, &after_len);
        if (!status) status |= buffer_after_terminator(val_str, after_len, '|', &type_str, &after_len);
        if (status == 0) {
            // Convert the type
            switch (*type_str) {
                case 'c':
                    type = COUNTER;
                    break;
                case 'm':
                    type = TIMER;
                    break;
                case 'k':
                case 'g':
                    type = KEY_VAL;
                    break;
                default:
                    type = UNKNOWN;
            }

            // Convert the value to a double
            endptr = NULL;
            val = strtod(val_str, &endptr);

            // Store the sample if we did the conversion
            if (val != 0 || endptr != val_str) {
                if (GLOBAL_CONFIG->input_counter != NULL)
                    metrics_add_sample(GLOBAL_METRICS, COUNTER, GLOBAL_CONFIG->input_counter, 1);

                metrics_add_sample(GLOBAL_METRICS, type, buf, val);
            } else {
                syslog(LOG_WARNING, "Failed value conversion! Input: %s", val_str);
            }
        } else {
            syslog(LOG_WARNING, "Failed parse metric! Input: %s", buf);
        }

        // Make sure to free the command buffer if we need to
        if (should_free) free(buf);
    }

    return 0;
}


/**
 * Invoked to handle binary commands.
 * @arg handle The connection related information
 * @return 0 on success.
 */
static int handle_binary_client_connect(statsite_conn_handler *handle) {
    metric_type type;
    int status, should_free;
    char *key;
    unsigned char header[BINARY_HEADER_SIZE];
    uint8_t type_input;
    double val;
    uint16_t key_len;
    while (1) {
        // Peek and check for the header. This is 12 bytes.
        // Magic byte - 1 byte
        // Metric type - 1 byte
        // Key length - 2 bytes
        // Metric value - 8 bytes
        status = peek_client_bytes(handle->conn, BINARY_HEADER_SIZE, (char*)&header);
        if (status == -1) return 0; // Return if no command is available

        // Check for the magic byte
        if (header[0] != BINARY_MAGIC_BYTE) {
            syslog(LOG_WARNING, "Received command from binary stream without magic byte!");
            return -1;
        }

        // Get the metric type
        type_input = header[1];
        switch (type_input) {
            case BIN_TYPE_KV:
                type = KEY_VAL;
                break;
            case BIN_TYPE_COUNTER:
                type = COUNTER;
                break;
            case BIN_TYPE_TIMER:
                type = TIMER;
                break;
            default:
                type = UNKNOWN;
                syslog(LOG_WARNING, "Received command from binary stream with unknown type: %u!", type_input);
                break;
        }

        // Extract the key length and value
        memcpy(&key_len, &header[2], 2);
        memcpy(&val, &header[4], 8);

        // Abort if we haven't received the full key, wait for the data
        if (available_bytes(handle->conn) < BINARY_HEADER_SIZE + key_len)
            return 0;

        // Seek past the header
        seek_client_bytes(handle->conn, BINARY_HEADER_SIZE);

        // Read the key now
        read_client_bytes(handle->conn, key_len, &key, &should_free);

        // Verify the key contains a null terminator
        if (memchr(key, '\0', key_len) == NULL) {
            syslog(LOG_WARNING, "Received command from binary stream with non-null terminated key: %.*s!", key_len, key);

            // For safety, we will just set the last byte to be null, and continue processing
            *(key + key_len - 1) = 0;
        }

        // Add the sample
        if (GLOBAL_CONFIG->input_counter != NULL)
            metrics_add_sample(GLOBAL_METRICS, COUNTER, GLOBAL_CONFIG->input_counter, 1);

        metrics_add_sample(GLOBAL_METRICS, type, key, val);

        // Make sure to free the command buffer if we need to
        if (should_free) free(key);
    }

    return 0;
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

