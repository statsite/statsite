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

