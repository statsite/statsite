#include <inttypes.h>
#include <string.h>

#include "binary_types.h"
#include "metrics.h"
#include "sink.h"
#include "streaming.h"

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


struct config_time {
    const statsite_config* global_config;
    struct timeval* tv;
};

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

static int stream_bin_writer(FILE *pipe, uint64_t timestamp, const statsite_config* config, unsigned char type,
        unsigned char val_type, double val, char *name) {
        char *prefix = NULL;
        uint16_t pre_len = 0;
        if (config->prefix_binary_stream) {
            prefix = config->prefixes_final[BIN_TYPE_MAP[type]];
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
#define STREAM_BIN(...) if (stream_bin_writer(pipe, ct->tv->tv_sec, ct->global_config, __VA_ARGS__, name)) return 1;
#define STREAM_UINT(val) if (!fwrite(&val, sizeof(unsigned int), 1, pipe)) return 1;

    timer_hist *t;
    int i;
    struct config_time* ct = data;
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
            STREAM_BIN(BIN_TYPE_COUNTER, BIN_OUT_RATE, counter_sum(value) / ct->global_config->flush_interval);
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
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_RATE, timer_sum(&t->tm) / ct->global_config->flush_interval);
            STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_SAMPLE_RATE, (double)timer_count(&t->tm) / ct->global_config->flush_interval);
            for (i=0; i < ct->global_config->num_quantiles; i++) {
                STREAM_BIN(BIN_TYPE_TIMER, BIN_OUT_PCT |
                    (int)(ct->global_config->quantiles[i] * 100),
                    timer_query(&t->tm, ct->global_config->quantiles[i]));
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
 * Streaming callback to format our output
 */
static int stream_formatter(FILE *pipe, void *data, metric_type type, char *name, void *value) {
    #define STREAM(...) if (fprintf(pipe, __VA_ARGS__, (long long)tv->tv_sec) < 0) return 1;
    struct config_time* ct = data;
    struct timeval *tv = ct->tv;
    timer_hist *t;
    int i;
    char *prefix = ct->global_config->prefixes_final[type];
    switch (type) {
        case KEY_VAL:
            STREAM("%s%s|%f|%lld\n", prefix, name, *(double*)value);
            break;

        case GAUGE:
            STREAM("%s%s|%f|%lld\n", prefix, name, ((gauge_t*)value)->value);
            break;

        case COUNTER:
            if (ct->global_config->extended_counters) {
                STREAM("%s%s.count|%" PRIu64 "|%lld\n", prefix, name, counter_count(value));
                STREAM("%s%s.mean|%f|%lld\n", prefix, name, counter_mean(value));
                STREAM("%s%s.stdev|%f|%lld\n", prefix, name, counter_stddev(value));
                STREAM("%s%s.sum|%f|%lld\n", prefix, name, counter_sum(value));
                STREAM("%s%s.sum_sq|%f|%lld\n", prefix, name, counter_squared_sum(value));
                STREAM("%s%s.lower|%f|%lld\n", prefix, name, counter_min(value));
                STREAM("%s%s.upper|%f|%lld\n", prefix, name, counter_max(value));
                STREAM("%s%s.rate|%f|%lld\n", prefix, name, counter_sum(value) / ct->global_config->flush_interval);
            } else {
                STREAM("%s%s|%f|%lld\n", prefix, name, counter_sum(value));
            }
            break;

        case SET:
            STREAM("%s%s|%" PRIu64 "|%lld\n", prefix, name, set_size(value));
            break;

        case TIMER:
            t = (timer_hist*)value;
            STREAM("%s%s.sum|%f|%lld\n", prefix, name, timer_sum(&t->tm));
            STREAM("%s%s.sum_sq|%f|%lld\n", prefix, name, timer_squared_sum(&t->tm));
            STREAM("%s%s.mean|%f|%lld\n", prefix, name, timer_mean(&t->tm));
            STREAM("%s%s.lower|%f|%lld\n", prefix, name, timer_min(&t->tm));
            STREAM("%s%s.upper|%f|%lld\n", prefix, name, timer_max(&t->tm));
            STREAM("%s%s.count|%" PRIu64 "|%lld\n", prefix, name, timer_count(&t->tm));
            STREAM("%s%s.stdev|%f|%lld\n", prefix, name, timer_stddev(&t->tm));
            for (i=0; i < ct->global_config->num_quantiles; i++) {
                if (ct->global_config->quantiles[i] == 0.5) {
                    STREAM("%s%s.median|%f|%lld\n", prefix, name, timer_query(&t->tm, 0.5));
                }
                STREAM("%s%s.p%0.0f|%f|%lld\n", prefix, name,
                    ct->global_config->quantiles[i] * 100,
                    timer_query(&t->tm, ct->global_config->quantiles[i]));
            }
            STREAM("%s%s.rate|%f|%lld\n", prefix, name, timer_sum(&t->tm) / ct->global_config->flush_interval);
            STREAM("%s%s.sample_rate|%f|%lld\n", prefix, name, (double)timer_count(&t->tm) / ct->global_config->flush_interval);

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

static int wrap_stream(struct sink* sink, metrics* m, void* data) {
    sink_config_stream *sc = (sink_config_stream*)sink->sink_config;
    stream_callback cb = (sc->binary_stream) ? stream_formatter_bin : stream_formatter;
    struct config_time ct = {
        .tv = data,
        .global_config = sink->global_config
    };
    return stream_to_command(m, &ct, cb, sc->stream_cmd);
}

/**
 * Build a stream sink - this is a basic form which has no actual parameters
 * and simply stores both configs.
 */
sink* init_stream_sink(const sink_config_stream* sc, const statsite_config* config) {
    sink* ss = calloc(1, sizeof(sink));
    ss->sink_config = (const sink_config*)sc;
    ss->global_config = config;
    ss->command = wrap_stream;
    return (sink*)ss;
}
