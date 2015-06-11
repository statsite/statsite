#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <jansson.h>
#include <curl/curl.h>

#include "metrics.h"
#include "sink.h"
#include "strbuf.h"



struct http_sink {
    sink sink;
};

/*
 * Data from the metrics_iter callback */
struct cb_info {
    json_t* jobject;
    const statsite_config* config;
    const sink_config_http *httpconfig;
};

/*
 * TODO: There is a lot redundant code here with sink_stream to normalize
 * an output representation of a metrics.
 */
static int add_metrics(void* data,
                       metric_type type,
                       char* name,
                       void* value) {

    struct cb_info* info = (struct cb_info*)data;
    json_t* obj = info->jobject;
    const statsite_config* config = info->config;

    /*
     * Scary looking macro to apply suffixes to a string, and then
     * insert them into a json object with a given value. Needs "suffixed"
     * in scope, with a "base_len" telling it how mmany chars the string is
     */
#define SUFFIX_ADD(suf, val)                                            \
    do {                                                                \
        suffixed[base_len - 1] = '\0';                                  \
        strcat(suffixed, suf);                                          \
        json_object_set_new(obj, suffixed, val);                        \
    } while(0)

    char* prefix = config->prefixes_final[type];
    /* Using C99 stack allocation, don't panic */
    int base_len = strlen(name) + strlen(prefix) + 1;
    char full_name[base_len];
    strcpy(full_name, prefix);
    strcat(full_name, name);
    switch (type) {
    case KEY_VAL:
        json_object_set_new(obj, full_name, json_real(*(double*)value));
        break;
    case GAUGE:
        json_object_set_new(obj, full_name, json_real(*(double*)value));
        break;
    case COUNTER:
    {
        if (config->extended_counters) {
            /* We allow up to 8 characters for a suffix, based on the static strings below */
            const int suffix_space = 8;
            char suffixed[base_len + suffix_space];
            strcpy(suffixed, full_name);
            SUFFIX_ADD(".count", json_integer(counter_count(value)));
            SUFFIX_ADD(".mean", json_real(counter_mean(value)));
            SUFFIX_ADD(".stdev", json_real(counter_stddev(value))); /* stdev matches other output */
            SUFFIX_ADD(".sum", json_real(counter_sum(value)));
            SUFFIX_ADD(".sum_sq", json_real(counter_squared_sum(value)));
            SUFFIX_ADD(".lower", json_real(counter_min(value)));
            SUFFIX_ADD(".upper", json_real(counter_max(value)));
            SUFFIX_ADD(".rate", json_real(counter_sum(value) / config->flush_interval));
        } else {
            json_object_set_new(obj, full_name, json_real(counter_sum(value)));
        }
        break;
    }
    case SET:
        json_object_set_new(obj, full_name, json_integer(set_size(value)));
        break;
    case TIMER:
    {
        timer_hist *t = (timer_hist*)value;
        /* We allow up to 40 characters for the metric name suffix. */
        const int suffix_space = 40;
        char suffixed[base_len + suffix_space];
        strcpy(suffixed, full_name);
        SUFFIX_ADD(".sum", json_real(timer_sum(&t->tm)));
        SUFFIX_ADD(".sum_sq", json_real(timer_squared_sum(&t->tm)));
        SUFFIX_ADD(".mean", json_real(timer_mean(&t->tm)));
        SUFFIX_ADD(".lower", json_real(timer_min(&t->tm)));
        SUFFIX_ADD(".upper", json_real(timer_max(&t->tm)));
        SUFFIX_ADD(".count", json_integer(timer_count(&t->tm)));
        SUFFIX_ADD(".stdev", json_real(timer_stddev(&t->tm))); /* stdev matches other output */
        for (int i = 0; i < config->num_quantiles; i++) {
            char ptile[suffix_space];
            snprintf(ptile, suffix_space, ".p%0.0f", config->quantiles[i] * 100);
            ptile[suffix_space-1] = '\0';
            SUFFIX_ADD(ptile, json_real(timer_query(&t->tm, config->quantiles[i])));
        }
        SUFFIX_ADD(".rate", json_real(timer_sum(&t->tm) / config->flush_interval));
        SUFFIX_ADD(".sample_rate", json_real((double)timer_count(&t->tm) / config->flush_interval));

        /* Manual histogram bins */
        if (t->conf) {
            char ptile[suffix_space];
            snprintf(ptile, suffix_space, ".bin_<%0.2f", t->conf->min_val);
            ptile[suffix_space-1] = '\0';
            SUFFIX_ADD(ptile, json_integer(t->counts[0]));
            for (int i = 0; i < t->conf->num_bins - 2; i++) {
                sprintf(ptile, ".bin_%0.2f", t->conf->min_val+(t->conf->bin_width*i));
                SUFFIX_ADD(ptile, json_integer(t->counts[i+1]));
            }
            sprintf(ptile, ".bin_>%0.2f", t->conf->max_val);
            SUFFIX_ADD(ptile, json_integer(t->counts[t->conf->num_bins - 1]));
        }
        break;
    }
    default:
        syslog(LOG_ERR, "Unknown metric type: %d", type);
        break;
    }

    return 0;
}

static int json_cb(const char* buf, size_t size, void* d) {
    strbuf* sbuf = (strbuf*)d;
    strbuf_cat(sbuf, buf, size);
    return 0;
}

static int serialize_metrics(struct http_sink* sink, metrics* m, void* data) {
    json_t* jobject = json_object();
    const sink_config_http* httpconfig = (const sink_config_http*)sink->sink.sink_config;

    struct cb_info info = {
        .jobject = jobject,
        .config = sink->sink.global_config,
        .httpconfig = httpconfig
    };
    /* produce a metrics json object */
    metrics_iter(m, &info, add_metrics);

    strbuf* json_buf;
    strbuf_new(&json_buf, 0);
    json_dump_callback(jobject, json_cb, (void*)json_buf, 0);

    int json_len = 0;
    char* json_data = strbuf_get(json_buf, &json_len);
    json_decref(jobject);

    /* Start working on the post buffer contents */
    strbuf* post_buf;
    strbuf_new(&post_buf, 128);

    CURL* curl = curl_easy_init();

    /* Encode the json document as a parameter */
    char *escaped_json_data = curl_easy_escape(curl, json_data, json_len);
    strbuf_catsprintf(post_buf, "%s=%s", httpconfig->metrics_name, escaped_json_data);
    strbuf_free(json_buf, true);
    curl_free(escaped_json_data);

    /* Encode the time stamp */
    struct timeval* tv = (struct timeval*) data;
    struct tm tm;
    gmtime_r(&tv->tv_sec, &tm);
    char time_buf[200];
    strftime(time_buf, 200, httpconfig->timestamp_format, &tm);
    char* encoded_time = curl_easy_escape(curl, time_buf, 0);
    strbuf_catsprintf(post_buf, "&%s=%s", httpconfig->timestamp_name, encoded_time);
    curl_free(encoded_time);

    /* Encode all the free-form parameters from configuration */
    for (kv_config* kv = httpconfig->params; kv != NULL; kv = kv->next) {
        char* encoded = curl_easy_escape(curl, kv->v, 0);
        strbuf_catsprintf(post_buf, "&%s=%s", kv->k, encoded);
        curl_free(encoded);
    }

    curl_easy_cleanup(curl);

    int post_len = 0;
    char* post_data = strbuf_get(post_buf, &post_len);
    strbuf_free(post_buf, false);

    /* TODO: Leave debugging in place for now */
    fprintf(stderr, "%s\n", post_data);
    fflush(stderr);

    return 0;
}

sink* init_http_sink(const sink_config_http* sc, const statsite_config* config) {
    struct http_sink* s = calloc(1, sizeof(struct http_sink));
    s->sink.sink_config = (const sink_config*)sc;
    s->sink.global_config = config;
    s->sink.command = (int (*)(sink*, metrics*, void*))serialize_metrics;
    return (sink*)s;
}
