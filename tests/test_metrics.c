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
    int res = init_metrics(0.01, (double*)&quants, 3, &m);
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

    fail_unless(metrics_add_sample(&m, COUNTER, "foo", 4) == 0);
    fail_unless(metrics_add_sample(&m, COUNTER, "foo", 6) == 0);
    fail_unless(metrics_add_sample(&m, COUNTER, "bar", 10) == 0);
    fail_unless(metrics_add_sample(&m, COUNTER, "bar", 20) == 0);

    fail_unless(metrics_add_sample(&m, TIMER, "baz", 1) == 0);
    fail_unless(metrics_add_sample(&m, TIMER, "baz", 10) == 0);

    int okay = 0;
    fail_unless(metrics_iter(&m, (void*)&okay, iter_test_all_cb) == 0);
    fail_unless(okay == 31);

    res = destroy_metrics(&m);
    fail_unless(res == 0);
}
END_TEST

