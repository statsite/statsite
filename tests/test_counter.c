#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <math.h>
#include "counter.h"

START_TEST(test_counter_init)
{
    counter c;
    int res = init_counter(&c);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_counter_init_add)
{
    counter c;
    int res = init_counter(&c);
    fail_unless(res == 0);

    fail_unless(counter_add_sample(&c, 100) == 0);
    fail_unless(counter_count(&c) == 1);
    fail_unless(counter_sum(&c) == 100);
    fail_unless(counter_mean(&c) == 100);
    fail_unless(counter_stddev(&c) == 0);
    fail_unless(counter_squared_sum(&c) == 10000);
    fail_unless(counter_min(&c) == 100);
    fail_unless(counter_max(&c) == 100);
}
END_TEST

START_TEST(test_counter_add_loop)
{
    counter c;
    int res = init_counter(&c);
    fail_unless(res == 0);

    for (int i=1; i<=100; i++)
        fail_unless(counter_add_sample(&c, i) == 0);

    fail_unless(counter_count(&c) == 100);
    fail_unless(counter_sum(&c) == 5050);
    fail_unless(counter_mean(&c) == 50.5);
    fail_unless(round(counter_stddev(&c)*1000)/1000 == 29.011);
    fail_unless(counter_squared_sum(&c) == 338350);
    fail_unless(counter_min(&c) == 1);
    fail_unless(counter_max(&c) == 100);
}
END_TEST

