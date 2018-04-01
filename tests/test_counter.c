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

    fail_unless(counter_add_sample(&c, 100, 1.0) == 0);
    fail_unless(counter_sum(&c) == 100);
}
END_TEST

START_TEST(test_counter_add_loop)
{
    counter c;
    int res = init_counter(&c);
    fail_unless(res == 0);

    for (int i=1; i<=100; i++)
        fail_unless(counter_add_sample(&c, i, 1.0) == 0);

    fail_unless(counter_sum(&c) == 5050);
}
END_TEST

START_TEST(test_counter_sample_rate)
{
    counter c;
    int res = init_counter(&c);
    fail_unless(res == 0);

    for (int i=1; i<=100; i++)
        fail_unless(counter_add_sample(&c, i, 0.5) == 0);

    fail_unless(counter_sum(&c) == 5050);
}
END_TEST
