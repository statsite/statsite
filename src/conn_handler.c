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
#include "metrics.h"
#include "streaming.h"
#include "conn_handler.h"
#include <inttypes.h>
#include "ascii_parser.h"

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
#define BIN_OUT_SAMPLE_RATE     0xc
#define BIN_OUT_PCT     0x80

// Macro to provide branch meta-data
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

// BIN_TYPE_MAP is used to map the BIN_TYPE_* static
// definitions into the metric_type value, so that
// we can lookup the proper prefix.
int BIN_TYPE_MAP[METRIC_TYPES] = {
    UNKNOWN,
    KEY_VAL,
    COUNTER,
    TIMER,
    SET,
    GAUGE,
    GAUGE_DELTA,
    };

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

void emit_stat(metric_type type,
    token *name, token *value, token *samplerate);

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
 * Streaming callback to format our output
 */
static int stream_formatter(FILE *pipe, void *data, metric_type type, char *name, void *value) {
    #define STREAM(...) if (fprintf(pipe, __VA_ARGS__, (long long)tv->tv_sec) < 0) return 1;
    #define STREAM_N(N, ...) if (fprintf(pipe, __VA_ARGS__, (long long)tv->tv_sec) < 0) return 1;
    struct timeval *tv = data;
    timer_hist *t;
    int i;
    char *prefix = GLOBAL_CONFIG->prefixes_final[type];
    included_metrics_config* timers_config = &(GLOBAL_CONFIG->timers_config);

    switch (type) {
        case KEY_VAL:
            STREAM("%s%s|%f|%lld\n", prefix, name, *(double*)value);
            break;

        case GAUGE:
            STREAM("%s%s|%f|%lld\n", prefix, name, ((gauge_t*)value)->value);
            break;

        case COUNTER:
            if (GLOBAL_CONFIG->extended_counters) {
                char *n = metric_name_format(name, ".count");
                if (GLOBAL_CONFIG->legacy_extended_counters) {
                    STREAM("%s%s|%"PRIu64"|%lld\n", prefix, n, counter_count(value));
                } else {
                    STREAM("%s%s|%f|%lld\n", prefix, n, counter_sum(value));
                }
                n = metric_name_format(name, ".rate");
                STREAM("%s%s|%f|%lld\n", prefix, n, counter_sum(value) / GLOBAL_CONFIG->flush_interval);
            } else {
                STREAM("%s%s|%f|%lld\n", prefix, name, counter_sum(value));
            }
            break;

        case SET:
            STREAM("%s%s|%"PRIu64"|%lld\n", prefix, name, set_size(value));
            break;

        case TIMER:
            t = (timer_hist*)value;
            if (timers_config->sum) {
                char *n = metric_name_format(name, ".sum");
                STREAM("%s%s|%f|%lld\n", prefix, n, timer_sum(&t->tm));
            }
            if (timers_config->sum_sq) {
                char *n = metric_name_format(name, ".sum_sq");
                STREAM("%s%s|%f|%lld\n", prefix, n, timer_squared_sum(&t->tm));
            }
            if (timers_config->mean) {
                char *n = metric_name_format(name, ".mean");
                STREAM("%s%s|%f|%lld\n", prefix, n, timer_mean(&t->tm));
            }
            if (timers_config->lower) {
                char *n = metric_name_format(name, ".lower");
                STREAM("%s%s|%f|%lld\n", prefix, n, timer_min(&t->tm));
            }
            if (timers_config->upper) {
                char *n = metric_name_format(name, ".upper");
                STREAM("%s%s|%f|%lld\n", prefix, n, timer_max(&t->tm));
            }
            if (timers_config->count) {
                char *n = metric_name_format(name, ".count");
                STREAM("%s%s|%"PRIu64"|%lld\n", prefix, n, timer_count(&t->tm));
            }
            if (timers_config->stdev) {
                char *n = metric_name_format(name, ".stdev");
                STREAM("%s%s|%f|%lld\n", prefix, n, timer_stddev(&t->tm));
            }
            for (i=0; i < GLOBAL_CONFIG->num_quantiles; i++) {
                char *n;
                double q = timer_query(&t->tm, GLOBAL_CONFIG->quantiles[i]);
                if (timers_config->median && GLOBAL_CONFIG->quantiles[i] == 0.5) {
                    n = metric_name_format(name, ".median");
                    STREAM("%s%s|%f|%lld\n", prefix, n, q);
                }
                n = metric_name_format_pcnt(name, ".p", GLOBAL_CONFIG->quantiles[i]);
                STREAM("%s%s|%f|%lld\n", prefix, n, q);
            }
            if (timers_config->rate) {
                char *n = metric_name_format(name, ".rate");
                STREAM("%s%s|%f|%lld\n", prefix, n, timer_sum(&t->tm) / GLOBAL_CONFIG->flush_interval);
            }
            if (timers_config->sample_rate) {
                char *n = metric_name_format(name, ".sample_rate");
                STREAM("%s%s|%f|%lld\n", prefix, n, (double)timer_count(&t->tm) / GLOBAL_CONFIG->flush_interval);
            }

            // Stream the histogram values
            if (t->conf) {
                char *n = metric_name_format_hist(name, ".histogram.bin_<", t->conf->min_val);
                STREAM("%s%s|%u|%lld\n", prefix, n, t->counts[0]);
                for (i=0; i < t->conf->num_bins-2; i++) {
                    n = metric_name_format_hist(name, ".histogram.bin_", t->conf->min_val+(t->conf->bin_width*i));
                    STREAM("%s%s|%u|%lld\n", prefix, n, t->counts[i+1]);
                }
                n = metric_name_format_hist(name, ".histogram.bin_>", t->conf->max_val);
                STREAM("%s%s|%u|%lld\n", prefix, n, t->counts[i+1]);
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
        char *prefix = NULL;
        uint16_t pre_len = 0;
        if (GLOBAL_CONFIG->prefix_binary_stream) {
            prefix = GLOBAL_CONFIG->prefixes_final[BIN_TYPE_MAP[type]];
            pre_len = strlen(prefix);
        }
        uint16_t key_len = strlen(name);
        uint16_t tot_len = pre_len + key_len + 1;
        struct binary_out_prefix out = {timestamp, type, val_type, tot_len, val};
        if (!fwrite(&out, sizeof(struct binary_out_prefix), 1, pipe)) return 1;
        if (pre_len > 0) {
            if (!fwrite(prefix, pre_len, 1, pipe)) return 1;
        }
        if (!fwrite(name, key_len + 1, 1, pipe)) return 1;
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
            if (GLOBAL_CONFIG->legacy_extended_counters) {
                STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_COUNT, counter_count(value));
            } else {
                STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_COUNT, counter_sum(value));
            }
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
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_MIN, timer_min(&t->tm));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_MAX, timer_max(&t->tm));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_COUNT, timer_count(&t->tm));
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_STDDEV, timer_stddev(&t->tm));

            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_RATE, timer_sum(&t->tm) / GLOBAL_CONFIG->flush_interval);
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_SAMPLE_RATE, (double)timer_count(&t->tm) / GLOBAL_CONFIG->flush_interval);
            for (i=0; i < GLOBAL_CONFIG->num_quantiles; i++) {
                STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_PCT |
                    (int)(GLOBAL_CONFIG->quantiles[i] * 100),
                    timer_query(&t->tm, GLOBAL_CONFIG->quantiles[i]));
            }

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
    return NULL;
}

/**
 * Invoked to when we've reached the flush interval timeout
 */
void flush_interval_trigger() {
    // Make a new metrics object
    metrics *m = malloc(sizeof(metrics));
    init_metrics(GLOBAL_CONFIG->timer_eps, GLOBAL_CONFIG->quantiles,
            GLOBAL_CONFIG->num_quantiles, GLOBAL_CONFIG->histograms,
            GLOBAL_CONFIG->set_precision, m);

    // Swap with the new one
    metrics *old = GLOBAL_METRICS;
    GLOBAL_METRICS = m;

    // Start a flush thread
    pthread_t thread;
    sigset_t oldset;
    pthread_attr_t attr;
    sigset_t newset;
    sigfillset(&newset);
    pthread_sigmask(SIG_BLOCK, &newset, &oldset);

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 256 * 1024);

    int err = pthread_create(&thread, &attr, flush_thread, old);
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);

    if (err == 0) {
        pthread_detach(thread);
        return;
    }

    syslog(LOG_WARNING, "Failed to spawn flush thread: %s", strerror(err));
    GLOBAL_METRICS = old;
    destroy_metrics(m);
    free(m);
}

/**
 * Called when statsite is terminating to flush the
 * final set of metrics
 */
void final_flush() {
    // Get the last set of metrics
    metrics *old = GLOBAL_METRICS;
    GLOBAL_METRICS = NULL;
    flush_thread(old);
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
    const char *orig_s = s;

    switch (*s) {
        case '-':
            neg = 1;
        case '+':
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
    if (unlikely(*s == 'E' || *s == 'e')) {
        errno = 0;
        return strtod(orig_s, end);
    }
    if (neg) val *= -1.0;
    if (end) *end = (char*)s;
    errno = 0;
    return val;
}

void emit_stat(metric_type type,
		token *name, token *value, token *samplerate)
{
    double val = 0.0, sample_rate = 1.0;
    char *endptr;

    if (type == GAUGE) {
        // Check if this is a delta update
        if (value->start[0] == '+' || value->start[0] == '-') {
            type = GAUGE_DELTA;
        }
    }
    // Increment the number of inputs received
    if (GLOBAL_CONFIG->input_counter)
    metrics_add_sample(GLOBAL_METRICS, COUNTER, GLOBAL_CONFIG->input_counter, 1, sample_rate);

    name->start[name->len] = '\0';
    value->start[value->len] = '\0';

    // Fast track the set-updates
    if (type == SET) {
        metrics_set_update(GLOBAL_METRICS, name->start, value->start);
		    return;
    }

    // Convert the value to a double
    val = str2double(value->start, &endptr);
    if (unlikely(endptr == value->start || errno == ERANGE)) {
        syslog(LOG_WARNING, "Failed value conversion! Input: %.*s", value->len, value->start);
        return;
    }

    // Handle counter/timer sampling if applicable
    if ((type == COUNTER || type == TIMER) && samplerate->len > 1) {
        double unchecked_rate = str2double(samplerate->start, &endptr);
        if (unlikely(endptr == samplerate->start)) {
            syslog(LOG_WARNING, "Failed sample rate conversion! Input: %.*s", samplerate->len, samplerate->start);
            return;
        }
        if (likely(unchecked_rate > 0 && unchecked_rate <= 1)) {
            sample_rate = unchecked_rate;
            if (type == COUNTER) {
                 val = val * (1.0 / sample_rate);
            }
        }
    }

    // Store the sample
    metrics_add_sample(GLOBAL_METRICS, type, name->start, val, sample_rate);
}

/**
 * Invoked to handle ASCII commands. This is the default
 * mode for statsite, to be backwards compatible with statsd
 * @arg handle The connection related information
 * @return 0 on success.
 */
static int handle_ascii_client_connect(statsite_conn_handler *handle) {
    // Look for the next command line
    char *buf;
    int buf_len, should_free, status;

		ascpp ascii_parser = ascpp_init(emit_stat);
    while (1) {
        status = extract_to_terminator(handle->conn, '\n', &buf, &buf_len, &should_free);
        if (status == -1) return 0; // Return if no command is available

        buf[buf_len-1] = '\n';
        ascpp_exec(&ascii_parser, buf, buf_len);

        // Make sure to free the command buffer if we need to
        if (should_free) free(buf);
    }
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
        metrics_add_sample(GLOBAL_METRICS, COUNTER, GLOBAL_CONFIG->input_counter, 1, 1.0);

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

        // Check for UDP record separator inserted by UDP handler
        if (cmd[0] == '\n') {
            if (should_free)
                free(cmd);
            if (seek_client_bytes(handle->conn, 1))
                return 0;  // End of buffer, shouldn't happen
            if (peek_client_bytes(handle->conn, MIN_BINARY_HEADER_SIZE, (char**)&cmd, &should_free))
                return 0;  // found end of buffer
        }

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
            metrics_add_sample(GLOBAL_METRICS, COUNTER, GLOBAL_CONFIG->input_counter, 1, 1.0);

        // Add the sample
        metrics_add_sample(GLOBAL_METRICS, type, (char*)key, *(double*)(cmd+4), 1.0);

        // Make sure to free the command buffer if we need to
        if (unlikely(should_free)) free(cmd);
    }
    return 0;
ERR_RET:
    if (unlikely(should_free)) free(cmd);
    return -1;
}
