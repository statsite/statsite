#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "set.h"

START_TEST(test_set_init_destroy)
{
    set_t s;
    fail_unless(set_init(12, &s) == 0);
    fail_unless(set_destroy(&s) == 0);
}
END_TEST

START_TEST(test_set_add_size_exact)
{
    set_t s;
    fail_unless(set_init(12, &s) == 0);
    fail_unless(set_size(&s) == 0);

    char buf[100];
    for (int i=0; i < SET_MAX_EXACT; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        set_add(&s, (char*)&buf);
        fail_unless(set_size(&s) == i+1);
    }

    fail_unless(set_destroy(&s) == 0);
}
END_TEST

START_TEST(test_set_add_size_exact_dedup)
{
    set_t s;
    fail_unless(set_init(12, &s) == 0);
    fail_unless(set_size(&s) == 0);

    char buf[100];
    fail_unless(sprintf((char*)&buf, "test"));

    for (int i=0; i < SET_MAX_EXACT; i++) {
        set_add(&s, (char*)&buf);
        fail_unless(set_size(&s) == 1);
    }

    fail_unless(set_destroy(&s) == 0);
}
END_TEST

START_TEST(test_set_error_bound)
{
    // Precision 14 -> variance of 1%
    set_t s;
    fail_unless(set_init(14, &s) == 0);

    char buf[100];
    for (int i=0; i < 10000; i++) {
        fail_unless(sprintf((char*)&buf, "test%d", i));
        set_add(&s, (char*)&buf);
    }

    // Should be within 1 %
    uint64_t size = set_size(&s);
    fail_unless(size > 9900 && size < 10100);

    fail_unless(set_destroy(&s) == 0);
}
END_TEST


