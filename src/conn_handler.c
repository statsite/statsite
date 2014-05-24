#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <regex.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <math.h>
#include "metrics.h"
#include "streaming.h"
#include "conn_handler.h"

/*
 * Binary defines
 */
#define BIN_TYPE_KV             0x1
#define BIN_TYPE_COUNTER        0x2
#define BIN_TYPE_TIMER          0x3
#define BIN_TYPE_SET            0x4
#define BIN_TYPE_GAUGE          0x5
#define BIN_TYPE_GAUGE_DELTA    0x6

#define BIN_OUT_NO_TYPE 0x0
#define BIN_OUT_SUM     0x1
#define BIN_OUT_SUM_SQ  0x2
#define BIN_OUT_MEAN    0x3
#define BIN_OUT_COUNT   0x4
#define BIN_OUT_STDDEV  0x5
#define BIN_OUT_MIN     0x6
#define BIN_OUT_MAX     0x7
#define BIN_OUT_HIST_FLOOR    0x8
#define BIN_OUT_HIST_BIN      0x9
#define BIN_OUT_HIST_CEIL     0xa
#define BIN_OUT_RATE     0xb
#define BIN_OUT_PCT     0x80

// Macro to provide branch meta-data
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

/* Static method declarations */
static int handle_binary_client_connect(statsite_conn_handler *handle);
static int handle_ascii_client_connect(statsite_conn_handler *handle);
static int buffer_after_terminator(char *buf, int buf_len, char terminator, char **after_term, int *after_len);

/* These are the quantiles we track */
static const double QUANTILES[] = {0.5, 0.95, 0.99};
static const int NUM_QUANTILES = 3;

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
 * Invoked by the signal handler
 */
int bucket_stats_cb(void *data, const char *key, void *value) {
    metrics_bucket *bm = (metrics_bucket *)value;
    syslog(LOG_INFO, "Bucket: %s, Timestamp: %lld, Creation Time: %lld. With following Clients:", key,
            *(long long *)bm->time, *(long long *)bm->creation_time);
    hashmap_iter(bm->clients,bucket_clients_cb, NULL);
    return 0;
}
int bucket_clients_cb(void *data, const char *key, void *value) {
    syslog(LOG_INFO, ">>> client: %s", key);
    return 0;
}

/**
 * Invoked to initialize the conn handler layer.
 */
void init_conn_handler(statsite_config *config) {
    // Make the initial metrics object
    metrics *m = malloc(sizeof(metrics));
    int res = init_metrics(config->timer_eps, (double*)&QUANTILES, NUM_QUANTILES,
            config->histograms, config->set_precision, m);
    assert(res == 0);
    GLOBAL_METRICS = m;
    hashmap_init(0, &METRICS_BUCKETS);

    // Store the config
    GLOBAL_CONFIG = config;
}

/**
 * Streaming callback to format our output
 */
static int stream_formatter(FILE *pipe, void *data, metric_type type, char *name, void *value) {
    #define STREAM(...) if (fprintf(pipe, __VA_ARGS__, (long long)tv->tv_sec) < 0) return 1;
    struct timeval *tv = data;
    timer_hist *t;
    int i;
    char *prefix = GLOBAL_CONFIG->prefixes_final[type];
    switch (type) {
        case KEY_VAL:
            STREAM("%s%s|%f|%lld\n", prefix, name, *(double*)value);
            break;

        case GAUGE:
            STREAM("%s%s|%f|%lld\n", prefix, name, ((gauge_t*)value)->value);
            break;

        case COUNTER:
            if (GLOBAL_CONFIG->extended_counters) {
                STREAM("%s%s.count|%lld|%lld\n", prefix, name, counter_count(value));
                STREAM("%s%s.mean|%f|%lld\n", prefix, name, counter_mean(value));
                STREAM("%s%s.stdev|%f|%lld\n", prefix, name, counter_stddev(value));
                STREAM("%s%s.sum|%f|%lld\n", prefix, name, counter_sum(value));
                STREAM("%s%s.sum_sq|%f|%lld\n", prefix, name, counter_squared_sum(value));
                STREAM("%s%s.lower|%f|%lld\n", prefix, name, counter_min(value));
                STREAM("%s%s.upper|%f|%lld\n", prefix, name, counter_max(value));
                STREAM("%s%s.rate|%f|%lld\n", prefix, name, counter_sum(value) / GLOBAL_CONFIG->flush_interval);
            } else {
                STREAM("%s%s|%f|%lld\n", prefix, name, counter_sum(value));
            }
            break;

        case SET:
            STREAM("%s%s|%lld|%lld\n", prefix, name, set_size(value));
            break;

        case TIMER:
            t = (timer_hist*)value;
            STREAM("%s%s.sum|%f|%lld\n", prefix, name, timer_sum(&t->tm));
            STREAM("%s%s.sum_sq|%f|%lld\n", prefix, name, timer_squared_sum(&t->tm));
            STREAM("%s%s.mean|%f|%lld\n", prefix, name, timer_mean(&t->tm));
            STREAM("%s%s.lower|%f|%lld\n", prefix, name, timer_min(&t->tm));
            STREAM("%s%s.upper|%f|%lld\n", prefix, name, timer_max(&t->tm));
            STREAM("%s%s.count|%lld|%lld\n", prefix, name, timer_count(&t->tm));
            STREAM("%s%s.stdev|%f|%lld\n", prefix, name, timer_stddev(&t->tm));
            STREAM("%s%s.median|%f|%lld\n", prefix, name, timer_query(&t->tm, 0.5));
            STREAM("%s%s.p95|%f|%lld\n", prefix, name, timer_query(&t->tm, 0.95));
            STREAM("%s%s.p99|%f|%lld\n", prefix, name, timer_query(&t->tm, 0.99));
            STREAM("%s%s.rate|%f|%lld\n", prefix, name, timer_sum(&t->tm) / GLOBAL_CONFIG->flush_interval);

            // Stream the histogram values
            if (t->conf) {
                STREAM("%s%s.histogram.bin_<%0.2f|%u|%lld\n", prefix, name, t->conf->min_val, t->counts[0]);
                for (i=0; i < t->conf->num_bins-2; i++) {
                    STREAM("%s%s.histogram.bin_%0.2f|%u|%lld\n", prefix, name, t->conf->min_val+(t->conf->bin_width*i), t->counts[i+1]);
                }
                STREAM("%s%s.histogram.bin_>%0.2f|%u|%lld\n", prefix, name, t->conf->max_val, t->counts[i+1]);
            }
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
    #define STREAM_UINT(val) if (!fwrite(&val, sizeof(unsigned int), 1, pipe)) return 1;
    timer_hist *t;
    int i;
    switch (type) {
        case KEY_VAL:
            STREAM_BIN(BIN_TYPE_KV, BIN_OUT_NO_TYPE, *(double*)value);
            break;

        case GAUGE:
            STREAM_BIN(BIN_TYPE_GAUGE, BIN_OUT_NO_TYPE, ((gauge_t*)value)->value);
            break;

        case COUNTER:
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_SUM, counter_sum(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_SUM_SQ, counter_squared_sum(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_MEAN, counter_mean(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_COUNT, counter_count(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_STDDEV, counter_stddev(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_MIN, counter_min(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_MAX, counter_max(value));
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_RATE, counter_sum(value) / GLOBAL_CONFIG->flush_interval);
            break;

        case SET:
            STREAM_BIN(BIN_TYPE_SET, BIN_OUT_SUM, set_size(value));
            break;

        case TIMER:
            t = (timer_hist*)value;
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_SUM, timer_sum(&t->tm));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_SUM_SQ, timer_squared_sum(&t->tm));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_MEAN, timer_mean(&t->tm));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_COUNT, timer_count(&t->tm));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_STDDEV, timer_stddev(&t->tm));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_MIN, timer_min(&t->tm));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_MAX, timer_max(&t->tm));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_PCT | 50, timer_query(&t->tm, 0.5));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_PCT | 95, timer_query(&t->tm, 0.95));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_PCT | 99, timer_query(&t->tm, 0.99));

            // Binary streaming for histograms
            if (t->conf) {
                STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_HIST_FLOOR, t->conf->min_val);
                STREAM_UINT(t->counts[0]);
                for (i=0; i < t->conf->num_bins-2; i++) {
                    STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_HIST_BIN, t->conf->min_val+(t->conf->bin_width*i));
                    STREAM_UINT(t->counts[i+1]);
                }
                STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_HIST_CEIL, t->conf->max_val);
                STREAM_UINT(t->counts[i+1]);
            }
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
    metrics_bucket *bm = arg;
    struct timeval tv;

    if (!bm->for_global) {
        tv.tv_sec = (time_t)*(bm->time);
    } else {
        // Get the current time
        gettimeofday(&tv, NULL);
    }

    // Determine which callback to use
    stream_callback cb = (GLOBAL_CONFIG->binary_stream)? stream_formatter_bin: stream_formatter;

    // Stream the records
    int res = stream_to_command(bm->metrics, &tv, cb, GLOBAL_CONFIG->stream_cmd);
    if (res != 0) {
        syslog(LOG_WARNING, "Streaming command exited with status %d", res);
    }

    // Cleanup
    free_metrics_bucket(bm);
    return NULL;
}

/**
 * Invoked to when we've reached the flush interval timeout
 */
void flush_interval_trigger() {
    // Make metrics_bucket for GLOBAL_METRICS
    metrics_bucket *bm = create_metrics_bucket(1);
    // Make a new metrics object
    metrics *m_new = malloc(sizeof(metrics));
    init_metrics(GLOBAL_CONFIG->timer_eps, (double*)&QUANTILES, NUM_QUANTILES,
            GLOBAL_CONFIG->histograms, GLOBAL_CONFIG->set_precision, m_new);

    bm->metrics = GLOBAL_METRICS;
    GLOBAL_METRICS = m_new;

    // Start a flush thread
    pthread_t thread;
    pthread_create(&thread, NULL, flush_thread, bm);
    pthread_detach(thread);
}

/**
 * Called when statsite is terminating to flush the
 * final set of metrics
 */
void final_flush() {
    // Get the last set of metrics
    metrics_bucket *bm = create_metrics_bucket(1);
    bm->metrics = GLOBAL_METRICS;
    GLOBAL_METRICS = NULL;

    // Start a flush thread
    pthread_t thread;
    pthread_create(&thread, NULL, flush_thread, bm);

    // Wait for the thread to finish
    pthread_join(thread, NULL);
    // Flush all the buckets
    hashmap_iter(METRICS_BUCKETS, flush_bucket_cb, NULL);
}

/**
 * Use this to free metrics_bucket
 */
void free_metrics_bucket(metrics_bucket *bm) {
    if (!bm->for_global) {
        free(bm->time);
        free(bm->creation_time);
        hashmap_destroy(bm->clients);
    }
    destroy_metrics(bm->metrics);
    free(bm->metrics);
    free(bm);
}

metrics_bucket* create_metrics_bucket(int for_global) {
    metrics_bucket *bm = malloc(sizeof(metrics_bucket));
    struct timeval tv;
    if (for_global) {
        bm->for_global = 1;
        return bm;
    } else {
        bm->metrics = malloc(sizeof(metrics));
        hashmap_init(0, &(bm->clients));
        init_metrics(GLOBAL_CONFIG->timer_eps, (double*)&QUANTILES, NUM_QUANTILES,
                GLOBAL_CONFIG->histograms, GLOBAL_CONFIG->set_precision, bm->metrics);
        bm->time = malloc(sizeof(time_t));
        gettimeofday(&tv, NULL);
        bm->creation_time = malloc(sizeof(time_t));
        *(bm->creation_time) = tv.tv_sec;
        bm->for_global = 0;
    }
    return bm;
}

/** Called by hashmap_iter in final_flush for
 * flushing all the buckets
 */
int flush_bucket_cb(void *data, const char *key, void *value) {
    metrics_bucket *bm = (metrics_bucket *)value;
    syslog(LOG_INFO, "Flushing Bucket: %s with Timestamp: %lld Created at: %lld", key,
            *(long long *)bm->time, *(long long *)bm->creation_time);
    hashmap_delete(METRICS_BUCKETS, (char *)key);

    // Start a flush thread
    pthread_t thread;
    pthread_create(&thread, NULL, flush_thread, bm);

    // Wait for the thread to finish
    pthread_join(thread, NULL);
    return 0;
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
 * Simple string to double conversion
 */
static double str2double(const char *s, char **end) {
    double val = 0.0;
    char neg = 0;
    if (*s == '-') {
        neg = 1;
        s++;
    }
    for (; *s >= '0' && *s <= '9'; s++) {
        val = (val * 10.0) + (*s - '0');
    }
    if (*s == '.') {
        s++;
        double frac = 0.0;
        int digits = 0;
        for (; *s >= '0' && *s <= '9'; s++) {
            frac = (frac * 10.0) + (*s - '0');
            digits++;
        }
        val += frac / pow(10.0, digits);
    }
    if (neg) val *= -1.0;
    if (end) *end = (char*)s;
    return val;
}

/**
 * Invoked to handle ASCII commands. This is the default
 * mode for statsite, to be backwards compatible with statsd
 * @arg handle The connection related information
 * @return 0 on success.
 */
static int handle_ascii_client_connect(statsite_conn_handler *handle) {
    // Look for the next command line
    void *ignored;
    char *buf, *key, *val_str, *type_str, *sample_str, *endptr, *bucket, *client_str;
    char *client_received = NULL;
    metric_type type;
    metrics_bucket *bm;
    metrics *m;
    int buf_len, should_free, status, i, after_len, bucket_len;
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
            case '>':
                type = START_BUCKET;
                break;
            case '<':
                type = END_BUCKET;
                break;
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
                        // Advance past the + to avoid breaking str2double
                        val_str++;
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

        // Convert the value to a double
        val = str2double(val_str, &endptr);
        if (unlikely(endptr == val_str)) {
            syslog(LOG_WARNING, "Failed value conversion! Input: %s", val_str);
            goto ERR_RET;
        }

        // Handle START_BUCKET & END_BUCKET commands
        if (type == START_BUCKET) {
            if (!buffer_after_terminator(type_str, after_len, '|', &client_str, &after_len)) {
                client_received = client_str;
            }
            int res = hashmap_get(METRICS_BUCKETS, buf, (void**)&bm);
            // Bucket already exists
            if (res == 0) {
                if (client_received) {
                    if (!hashmap_get(bm->clients, client_received, (void **)&ignored)) {
                        syslog(LOG_WARNING, "Got START_BUCKET for an existing bucket: %s, from existing client: %s, ignoring...", buf, client_received);
                        goto END_LOOP;
                    } else {
                        syslog(LOG_DEBUG, "Got START_BUCKET for bucket: %s, from new client: %s", buf, client_received);
                        hashmap_put(bm->clients, client_received, NULL);
                        goto END_LOOP;
                    }
                } else {
                    syslog(LOG_WARNING, "Got START_BUCKET for an existing bucket: %s, ignoring...", buf);
                    goto END_LOOP;
                }
            }
            bm = create_metrics_bucket(0);
            *(bm->time) = (time_t)val;
            hashmap_put(METRICS_BUCKETS, buf, bm);
            if (client_received) hashmap_put(bm->clients, client_received, NULL);
            goto END_LOOP;
        }

        if (type == END_BUCKET) {
            int res = hashmap_get(METRICS_BUCKETS, buf, (void**)&bm);
            // Bucket doesn't exist
            if (res == -1) {
                syslog(LOG_WARNING, "Bucket doesn't exist hence can't flush it: %s", buf);
                goto END_LOOP;
            }
            if (!buffer_after_terminator(type_str, after_len, '|', &client_str, &after_len)) {
                syslog(LOG_DEBUG, "Got END_BUCKET for bucket: %s, from client: %s", buf, client_str);
                hashmap_delete(bm->clients, client_str);
            }
            if (hashmap_size(bm->clients) == 0) flush_bucket_cb(NULL, buf, (void *)bm);
            goto END_LOOP;
        }

        // Handle counter sampling if applicable
        if (type == COUNTER && !buffer_after_terminator(type_str, after_len, '@', &sample_str, &after_len)) {
            sample_rate = str2double(sample_str, &endptr);
            if (unlikely(endptr == sample_str)) {
                syslog(LOG_WARNING, "Failed sample rate conversion! Input: %s", sample_str);
                goto ERR_RET;
            }
            // if @ is found then we want to scan after that otherwise scan starts at type_str itself
            type_str = sample_str;
            if (sample_rate > 0 && sample_rate <= 1) {
                // Magnify the value
                val = val * (1.0 / sample_rate);
            }
        }

        // XXX:
        //  1. Support Binary Protocol
        //  2. May be HUP can be used for shedding the buckets off
        // Choose bucket based on flags
        if (type != START_BUCKET && type != END_BUCKET && !buffer_after_terminator(type_str, after_len, '>', &bucket, &bucket_len)) {
            int res = hashmap_get(METRICS_BUCKETS, bucket, (void**)&bm);
            // Bucket doesn't exist
            if (res == -1) {
                syslog(LOG_WARNING, "Couldn't find the bucket to update: %s", bucket);
                goto END_LOOP;
            }
            m = bm->metrics;
        } else {
            m = GLOBAL_METRICS;
        }

        // Increment the number of inputs received
        if (GLOBAL_CONFIG->input_counter)
            metrics_add_sample(m, COUNTER, GLOBAL_CONFIG->input_counter, 1);

        // Fast track the set-updates
        if (type == SET) {
            metrics_set_update(m, buf, val_str);
            goto END_LOOP;
        }

        // Store the sample
        metrics_add_sample(m, type, buf, val);

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
