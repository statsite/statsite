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

