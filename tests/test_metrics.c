#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>
#include "metrics.h"

START_TEST(test_metrics_init_and_destroy)
{
    metrics m;
    double quants[] = {0.5, 0.90, 0.99};
    int res = init_metrics(0.01, (double*)&quants, 3, NULL, 12, &m);
    fail_unless(res == 0);

    res = destroy_metrics(&m);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_metrics_init_defaults_and_destroy)
{
    metrics m;
    int res = init_metrics_defaults(&m);
    fail_unless(res == 0);

    res = destroy_metrics(&m);
    fail_unless(res == 0);
}
END_TEST

static int iter_cancel_cb(void *data, metric_type type, char *key, void *val) {
    return 1;
}

START_TEST(test_metrics_empty_iter)
{
    metrics m;
    int res = init_metrics_defaults(&m);
    fail_unless(res == 0);

    fail_unless(metrics_iter(&m, NULL, iter_cancel_cb) == 0);

    res = destroy_metrics(&m);
    fail_unless(res == 0);
}
END_TEST

static int iter_test_cb(void *data, metric_type type, char *key, void *val) {
    if (type == KEY_VAL && strcmp(key, "test") == 0) {
        double *v = val;
        if (*v == 100) {
            int *okay = data;
            *okay = 1;
        }
    }
    return 0;
}

START_TEST(test_metrics_add_iter)
{
    metrics m;
    int res = init_metrics_defaults(&m);
    fail_unless(res == 0);

    int okay = 0;
    fail_unless(metrics_add_sample(&m, KEY_VAL, "test", 100) == 0);
    fail_unless(metrics_iter(&m, (void*)&okay, iter_test_cb) == 0);
    fail_unless(okay == 1);

    res = destroy_metrics(&m);
    fail_unless(res == 0);
}
END_TEST

static int iter_test_all_cb(void *data, metric_type type, char *key, void *val) {
    int *o = data;
    switch (type) {
        case KEY_VAL:
            if (strcmp(key, "test") == 0 && *(double*)val == 100)
                *o = *o | 1;
            if (strcmp(key, "test2") == 0 && *(double*)val == 42)
                *o = *o | 1 << 1;
            break;
        case COUNTER:
            if (strcmp(key, "foo") == 0 && counter_sum(val) == 10)
                *o = *o | 1 << 2;
            if (strcmp(key, "bar") == 0 && counter_sum(val) == 30)
                *o = *o | 1 << 3;
            break;
        case TIMER:
            if (strcmp(key, "baz") == 0 && timer_sum(val) == 11)
                *o = *o | 1 << 4;
            break;
        case SET:
            if (strcmp(key, "zip") == 0 && set_size(val) == 2)
                *o = *o | 1 << 5;
            break;
        case GAUGE:
            if (strcmp(key, "g1") == 0 && ((gauge_t*)val)->value == 200)
                *o = *o | 1 << 6;
            if (strcmp(key, "g2") == 0 && ((gauge_t*)val)->value == 42)
                *o = *o | 1 << 7;
            break;
        default:
            return 1;
    }
    return 0;
}

START_TEST(test_metrics_add_all_iter)
{
    metrics m;
    int res = init_metrics_defaults(&m);
    fail_unless(res == 0);

    fail_unless(metrics_add_sample(&m, KEY_VAL, "test", 100) == 0);
    fail_unless(metrics_add_sample(&m, KEY_VAL, "test2", 42) == 0);

    fail_unless(metrics_add_sample(&m, GAUGE, "g1", 1) == 0);
    fail_unless(metrics_add_sample(&m, GAUGE, "g1", 200) == 0);
    fail_unless(metrics_add_sample(&m, GAUGE, "g2", 42) == 0);

    fail_unless(metrics_add_sample(&m, COUNTER, "foo", 4) == 0);
    fail_unless(metrics_add_sample(&m, COUNTER, "foo", 6) == 0);
    fail_unless(metrics_add_sample(&m, COUNTER, "bar", 10) == 0);
    fail_unless(metrics_add_sample(&m, COUNTER, "bar", 20) == 0);

    fail_unless(metrics_add_sample(&m, TIMER, "baz", 1) == 0);
    fail_unless(metrics_add_sample(&m, TIMER, "baz", 10) == 0);

    fail_unless(metrics_set_update(&m, "zip", "foo") == 0);
    fail_unless(metrics_set_update(&m, "zip", "wow") == 0);

    int okay = 0;
    fail_unless(metrics_iter(&m, (void*)&okay, iter_test_all_cb) == 0);
    fail_unless(okay == 255);

    res = destroy_metrics(&m);
    fail_unless(res == 0);
}
END_TEST

static int iter_test_histogram(void *data, metric_type type, char *key, void *val) {
    int *o = data;
    timer_hist *t = val;
    if (strcmp(key, "baz") == 0 && t->conf && t->counts) {
        // Verify the counts
        if (t->counts[0] == 1 && t->counts[1] == 1 && t->counts[6] == 1 && t->counts[11] == 1) {
            *o = *o | 1;
        }
    } else if (strcmp(key, "zip") == 0 && !t->conf && !t->counts) {
        *o = *o | 1 << 1;
    } else
        return 1;
    return 0;
}

START_TEST(test_metrics_histogram)
{
    statsite_config config;
    int res = config_from_filename(NULL, &config);

    // Build a histogram config
    histogram_config c1 = {"foo", 0, 200, 20, 12, NULL, 0};
    histogram_config c2 = {"baz", 0, 20, 2, 12, NULL, 0};
    config.hist_configs = &c1;
    c1.next = &c2;
    fail_unless(build_prefix_tree(&config) == 0);

    metrics m;
    double quants[] = {0.5, 0.90, 0.99};
    res = init_metrics(0.01, (double*)&quants, 3, config.histograms, 12, &m);
    fail_unless(res == 0);

    fail_unless(metrics_add_sample(&m, TIMER, "baz", 1) == 0);
    fail_unless(metrics_add_sample(&m, TIMER, "baz", 10) == 0);
    fail_unless(metrics_add_sample(&m, TIMER, "baz", -1) == 0);
    fail_unless(metrics_add_sample(&m, TIMER, "baz", 50) == 0);

    fail_unless(metrics_add_sample(&m, TIMER, "zip", 1) == 0);
    fail_unless(metrics_add_sample(&m, TIMER, "zip", 10) == 0);
    fail_unless(metrics_add_sample(&m, TIMER, "zip", -1) == 0);
    fail_unless(metrics_add_sample(&m, TIMER, "zip", 50) == 0);

    int okay = 0;
    fail_unless(metrics_iter(&m, (void*)&okay, iter_test_histogram) == 0);
    fail_unless(okay == 3);

    res = destroy_metrics(&m);
    fail_unless(res == 0);
}
END_TEST

static int iter_test_gauge(void *data, metric_type type, char *key, void *val) {
    int *o = data;
    if (strcmp(key, "g1") == 0 && ((gauge_t*)val)->value == 42) {
        *o = *o | 1;
    } else if (strcmp(key, "g2") == 0 && ((gauge_t*)val)->value == 100) {
        *o = *o | (1 << 1);
    } else if (strcmp(key, "g3") == 0 && ((gauge_t*)val)->value == -100) {
        *o = *o | (1 << 2);
    } else
        return 1;
    return 0;
}

START_TEST(test_metrics_gauges)
{
    metrics m;
    int res = init_metrics_defaults(&m);
    fail_unless(res == 0);

    fail_unless(metrics_add_sample(&m, GAUGE, "g1", 1) == 0);
    fail_unless(metrics_add_sample(&m, GAUGE_DELTA, "g1", 41) == 0);

    fail_unless(metrics_add_sample(&m, GAUGE_DELTA, "g2", 100) == 0);
    fail_unless(metrics_add_sample(&m, GAUGE_DELTA, "g3", -100) == 0);

    int okay = 0;
    fail_unless(metrics_iter(&m, (void*)&okay, iter_test_gauge) == 0);
    fail_unless(okay == 7);

    res = destroy_metrics(&m);
    fail_unless(res == 0);
}
END_TEST

