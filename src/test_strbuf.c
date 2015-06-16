#include <check.h>
#include <stdio.h>
#include "strbuf.h"

START_TEST(test_strbuf_new)
{
    strbuf* buf;
    strbuf_new(&buf, 0);
    fail_unless(buf != NULL);
    int len;
    char* d = strbuf_get(buf, &len);
    fail_unless(d != NULL);
    fail_unless(len == 0);
    strbuf_free(buf, true);
}
END_TEST

START_TEST(test_strbuf_printf)
{
    strbuf* buf;
    strbuf_new(&buf, 0);
    fail_unless(buf != NULL);
    int len;
    char* d = strbuf_get(buf, &len);
    fail_unless(d != NULL);
    fail_unless(len == 0);

    strbuf_catsprintf(buf, "hello world %s", "hello world");
    strbuf_catsprintf(buf, "hi");
    strbuf_cat(buf, "hu", 2);
    d = strbuf_get(buf, &len);
    fail_unless(d != NULL);
    fail_unless(len == 11 + 1 + 11 + 2 + 2);

    ck_assert_str_eq(d, "hello world hello worldhihu");

    strbuf_free(buf, true);
}
END_TEST

START_TEST(test_strbuf_big)
{
    strbuf* buf;
    strbuf_new(&buf, 0);
    fail_unless(buf != NULL);
    int len;
    char* d = strbuf_get(buf, &len);
    fail_unless(d != NULL);
    fail_unless(len == 0);

    for (int i = 0; i < 1000; i++)
        strbuf_catsprintf(buf, "hello world");

    d = strbuf_get(buf, &len);
    fail_unless(d != NULL);
    fail_unless(len == 1000 * 11);

    fail_unless(strncmp(d, "hello worldhello world", 22) == 0);

    strbuf_free(buf, true);
}
END_TEST
