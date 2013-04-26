#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "hll.h"

START_TEST(test_hll_init_bad)
{
    hll_t h;
    fail_unless(hll_init(HLL_MIN_PRECISION-1, &h) == -1);
    fail_unless(hll_init(HLL_MAX_PRECISION+1, &h) == -1);

    fail_unless(hll_init(HLL_MIN_PRECISION, &h) == 0);
    fail_unless(hll_destroy(&h) == 0);

    fail_unless(hll_init(HLL_MAX_PRECISION, &h) == 0);
    fail_unless(hll_destroy(&h) == 0);
}
END_TEST


START_TEST(test_hll_init_and_destroy)
{
    hll_t h;
    fail_unless(hll_init(10, &h) == 0);
    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_hll_add)
{
    hll_t h;
    fail_unless(hll_init(10, &h) == 0);

    char buf[100];
    for (int i=0; i < 100; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        hll_add(&h, (char*)&buf);
    }

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_hll_add_hash)
{
    hll_t h;
    fail_unless(hll_init(10, &h) == 0);

    char buf[100];
    for (uint64_t i=0; i < 100; i++) {
        hll_add_hash(&h, i ^ rand());
    }

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_hll_add_size)
{
    hll_t h;
    fail_unless(hll_init(10, &h) == 0);

    char buf[100];
    for (int i=0; i < 100; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        hll_add(&h, (char*)&buf);
    }

    double s = hll_size(&h);
    fail_unless(s > 95 && s < 105);

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_hll_size)
{
    hll_t h;
    fail_unless(hll_init(10, &h) == 0);

    double s = hll_size(&h);
    fail_unless(s == 0);

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_hll_error_bound)
{
    // Precision 14 -> variance of 1%
    hll_t h;
    fail_unless(hll_init(14, &h) == 0);

    char buf[100];
    for (int i=0; i < 10000; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        hll_add(&h, (char*)&buf);
    }

    // Should be within 1%
    double s = hll_size(&h);
    fail_unless(s > 9900 && s < 10100);

    fail_unless(hll_destroy(&h) == 0);
}
END_TEST

START_TEST(test_hll_precision_for_error)
{
    fail_unless(hll_precision_for_error(1.0) == -1);
    fail_unless(hll_precision_for_error(0.0) == -1);
    fail_unless(hll_precision_for_error(0.02) == 12);
    fail_unless(hll_precision_for_error(0.01) == 14);
    fail_unless(hll_precision_for_error(0.005) == 16);
}
END_TEST


