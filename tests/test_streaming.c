#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>
#include "streaming.h"

static int empty_cb(FILE *pipe, void *data, metric_type type, char *name, void *value) {
    int *o = data;
    *o = 1;
    return 1;
}

START_TEST(test_stream_empty)
{
    metrics m;
    int res = init_metrics_defaults(&m);
    fail_unless(res == 0);

    int called = 0;
    res = stream_to_command(&m, &called, empty_cb, "tee /dev/null");
    fail_unless(res == 0);
    fail_unless(called == 0);

    res = destroy_metrics(&m);
    fail_unless(res == 0);
}
END_TEST

