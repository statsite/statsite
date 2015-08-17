#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <jansson.h>
#include <curl/curl.h>
#include <pthread.h>
#include <unistd.h>

#include "lifoq.h"
#include "metrics.h"
#include "sink.h"
#include "strbuf.h"

const int QUEUE_MAX_SIZE = 10 * 1024 * 1024; /* 10 MB of data */
const int DEFAULT_TIMEOUT_SECONDS = 30;
const useconds_t FAILURE_WAIT = 5000000; /* 5 seconds */

const char* DEFAULT_CIPHERS_NSS = "ecdhe_ecdsa_aes_128_gcm_sha_256,ecdhe_rsa_aes_256_sha,rsa_aes_128_gcm_sha_256,rsa_aes_256_sha,rsa_aes_128_sha";
const char* DEFAULT_CIPHERS_OPENSSL = "EECDH+AESGCM:EDH+AESGCM:AES256+EECDH:AES256+EDH:DHE-RSA-AES256-SHA:DHE-RSA-AES128-SHA";

const char* USERAGENT = "statsite-http/0";
const char* OAUTH2_GRANT = "grant_type=client_credentials";

struct http_sink {
    sink sink;
    lifoq* queue;
    pthread_t worker;
    char* oauth_bearer;
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

    /* Many APIs reject empty metrics lists - in this case, we simply early abort */
    if (json_len == 2) {
        strbuf_free(json_buf, true);
        return 0;
    }

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
    localtime_r(&tv->tv_sec, &tm);
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

    int push_ret = lifoq_push(sink->queue, post_data, post_len, true, false);
    if (push_ret) {
        syslog(LOG_ERR, "HTTP Sink couldn't enqueue a %d size buffer - rejected code %d",
               post_len, push_ret);
    }

    return 0;
}

/*
 * libcurl data writeback handler - buffers into a growable buffer
 */
size_t recv_cb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    strbuf* buf = (strbuf*)userdata;
    /* Note: ptr is not NULL terminated, but strbuf_cat enforces a NULL */
    strbuf_cat(buf, ptr, size * nmemb);

    return size * nmemb;
}

/*
 * Attempt to check if this libcurl is using OpenSSL or NSS, which
 * differ in how ciphers are listed.
 */
static const char* curl_which_ssl(void) {
    curl_version_info_data* v = curl_version_info(CURLVERSION_NOW);
    syslog(LOG_NOTICE, "HTTP: libcurl is built with %s %s", v->version, v->ssl_version);
    if (v->ssl_version && strncmp(v->ssl_version, "NSS", 3) == 0)
        return DEFAULT_CIPHERS_NSS;
    else
        return DEFAULT_CIPHERS_OPENSSL;
}

static void http_curl_basic_setup(CURL* curl,
                                  const sink_config_http* httpconfig, struct curl_slist* headers,
                                  char* error_buffer,
                                  strbuf* recv_buf,
                                  const char* ssl_ciphers) {
    /* Setup HTTP parameters */
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, DEFAULT_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, recv_buf);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, recv_cb);
    curl_easy_setopt(curl, CURLOPT_SSL_CIPHER_LIST, ssl_ciphers);
    if (headers)
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USERAGENT);
}

/*
 * A helper to try to authenticate to an OAuth2 token endpoint
 */
static int oauth2_get_token(const sink_config_http* httpconfig, struct http_sink* sink) {
    char* error_buffer = malloc(CURL_ERROR_SIZE + 1);
    strbuf *recv_buf;
    strbuf_new(&recv_buf, 16384);

    const char* ssl_ciphers;
    if (httpconfig->ciphers)
        ssl_ciphers = httpconfig->ciphers;
    else
        ssl_ciphers = curl_which_ssl();


    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, httpconfig->oauth_token_url);
    http_curl_basic_setup(curl, httpconfig, NULL, error_buffer, recv_buf, ssl_ciphers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(OAUTH2_GRANT));
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, OAUTH2_GRANT);
    curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl, CURLOPT_USERNAME, httpconfig->oauth_key);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, httpconfig->oauth_secret);

    CURLcode rcurl = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    int recv_len;
    char* recv_data = strbuf_get(recv_buf, &recv_len);
    printf("DATA: %s\n", recv_data);
    if (http_code != 200 || rcurl != CURLE_OK) {
        syslog(LOG_ERR, "HTTP auth: error %d: %s %s", rcurl, error_buffer, recv_data);
        usleep(FAILURE_WAIT);
        goto exit;
    } else {
        json_error_t error;
        json_t* root = json_loadb(recv_data, recv_len, 0, &error);
        if (!root) {
            syslog(LOG_ERR, "HTTP auth: JSON load error: %s", error.text);
            goto exit;
        }
        char* token = NULL;
        if (json_unpack_ex(root, &error, 0, "{s:s}", "access_token", &token) != 0) {
            syslog(LOG_ERR, "HTTP auth: JSON unpack error: %s", error.text);
            json_decref(root);
            goto exit;
        }
        sink->oauth_bearer = strdup(token);
        json_decref(root);
        syslog(LOG_NOTICE, "HTTP auth: Got valid OAuth2 token");
    }

exit:

    curl_easy_cleanup(curl);
    free(error_buffer);
    strbuf_free(recv_buf, true);
    return 0;
}

/*
 * A simple background worker thread which pops from the queue and tries
 * to post. If the queue is marked closed, this thread exits
 */
static void* http_worker(void* arg) {
    struct http_sink* s = (struct http_sink*)arg;
    const sink_config_http* httpconfig = (sink_config_http*)s->sink.sink_config;

    char* error_buffer = malloc(CURL_ERROR_SIZE + 1);
    strbuf *recv_buf;

    const char* ssl_ciphers;
    if (httpconfig->ciphers)
        ssl_ciphers = httpconfig->ciphers;
    else
        ssl_ciphers = curl_which_ssl();

    syslog(LOG_NOTICE, "HTTP: Using cipher suite %s", ssl_ciphers);

    bool should_authenticate = httpconfig->oauth_key != NULL;

    syslog(LOG_NOTICE, "Starting HTTP worker");
    strbuf_new(&recv_buf, 16384);

    while(true) {
        void *data = NULL;
        size_t data_size = 0;
        int ret = lifoq_get(s->queue, &data, &data_size);
        if (ret == LIFOQ_CLOSED)
            goto exit;

        if (should_authenticate && s->oauth_bearer == NULL) {
            if (!oauth2_get_token(httpconfig, s)) {
                if (lifoq_push(s->queue, data, data_size, true, true))
                    syslog(LOG_ERR, "HTTP: dropped data due to queue full of closed");
                continue;
            }
        }

        memset(error_buffer, 0, CURL_ERROR_SIZE+1);
        CURL* curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_URL, httpconfig->post_url);

        /* Build headers */
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, "Connection: close");

        /* Add a bearer header if needed */
        if (should_authenticate) {
            /* 30 is header preamble + fluff */
            char bearer_header[30 + strlen(s->oauth_bearer)];
            sprintf(bearer_header, "Authorization: Bearer %s", s->oauth_bearer);
            headers = curl_slist_append(headers, bearer_header);
        }

        http_curl_basic_setup(curl, httpconfig, headers, error_buffer, recv_buf, ssl_ciphers);

        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data_size);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);

        syslog(LOG_NOTICE, "HTTP: Sending %zd bytes to %s", data_size, httpconfig->post_url);
        /* Do it! */
        CURLcode rcurl = curl_easy_perform(curl);
        long http_code = 0;

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code != 200 || rcurl != CURLE_OK) {

            int recv_len;
            char* recv_data = strbuf_get(recv_buf, &recv_len);

            syslog(LOG_ERR, "HTTP: error %d: %s %s", rcurl, error_buffer, recv_data);
            /* Re-enqueue data */
            if (lifoq_push(s->queue, data, data_size, true, true))
                syslog(LOG_ERR, "HTTP: dropped data due to queue full of closed");

            /* Remove any authentication token - this will cause us to get a new one */
            if (s->oauth_bearer) {
                free(s->oauth_bearer);
                s->oauth_bearer = NULL;
            }

            usleep(FAILURE_WAIT);
        } else {
            syslog(LOG_NOTICE, "HTTP: success");
            free(data);
        }

        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
        strbuf_truncate(recv_buf);
    }
exit:

    free(error_buffer);
    strbuf_free(recv_buf, true);
    return NULL;
}

static void close_sink(struct http_sink* s) {
    lifoq_close(s->queue);
    void* retval;
    pthread_join(s->worker, &retval);
    syslog(LOG_NOTICE, "HTTP: sink closed down with status %ld", (intptr_t)retval);
    return;
}

sink* init_http_sink(const sink_config_http* sc, const statsite_config* config) {
    struct http_sink* s = calloc(1, sizeof(struct http_sink));
    s->sink.sink_config = (const sink_config*)sc;
    s->sink.global_config = config;
    s->sink.command = (int (*)(sink*, metrics*, void*))serialize_metrics;
    s->sink.close = (void (*)(sink*))close_sink;

    lifoq_new(&s->queue, QUEUE_MAX_SIZE);
    pthread_create(&s->worker, NULL, http_worker, (void*)s);

    return (sink*)s;
}
