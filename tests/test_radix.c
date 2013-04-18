#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "radix.h"

START_TEST(test_radix_init_and_destroy)
{
    radix_tree t;
    fail_unless(radix_init(&t) == 0);
    fail_unless(radix_destroy(&t) == 0);
}
END_TEST

START_TEST(test_radix_insert)
{
    radix_tree t;
    fail_unless(radix_init(&t) == 0);

    void *val = NULL;
    fail_unless(radix_insert(&t, NULL, &val) == 0);
    fail_unless(radix_insert(&t, "", &val) == 1);
    fail_unless(radix_insert(&t, "foo", &val) == 0);
    fail_unless(radix_insert(&t, "bar", &val) == 0);
    fail_unless(radix_insert(&t, "baz", &val) == 0);
    fail_unless(radix_insert(&t, "f", &val) == 0);

    fail_unless(radix_destroy(&t) == 0);
}
END_TEST

START_TEST(test_radix_search)
{
    radix_tree t;
    fail_unless(radix_init(&t) == 0);

    void *val = (void*)1;
    fail_unless(radix_insert(&t, NULL, &val) == 0);
    val = (void*)2;
    fail_unless(radix_insert(&t, "", &val) == 1);
    val = (void*)3;
    fail_unless(radix_insert(&t, "foo", &val) == 0);
    val = (void*)4;
    fail_unless(radix_insert(&t, "bar", &val) == 0);
    val = (void*)5;
    fail_unless(radix_insert(&t, "baz", &val) == 0);
    val = (void*)6;
    fail_unless(radix_insert(&t, "f", &val) == 0);

    fail_unless(radix_search(&t, NULL, &val) == 0);
    fail_unless(val == (void*)2);
    fail_unless(radix_search(&t, "", &val) == 0);
    fail_unless(val == (void*)2);
    fail_unless(radix_search(&t, "foo", &val) == 0);
    fail_unless(val == (void*)3);
    fail_unless(radix_search(&t, "bar", &val) == 0);
    fail_unless(val == (void*)4);
    fail_unless(radix_search(&t, "baz", &val) == 0);
    fail_unless(val == (void*)5);
    fail_unless(radix_search(&t, "f", &val) == 0);
    fail_unless(val == (void*)6);

    fail_unless(radix_search(&t, "bad", &val) == 1);
    fail_unless(radix_search(&t, "tubez", &val) == 1);
    fail_unless(radix_search(&t, "fo", &val) == 1);

    fail_unless(radix_destroy(&t) == 0);
}
END_TEST


START_TEST(test_radix_longest_prefix)
{
    radix_tree t;
    fail_unless(radix_init(&t) == 0);

    void *val = (void*)1;
    fail_unless(radix_insert(&t, NULL, &val) == 0);
    val = (void*)2;
    fail_unless(radix_insert(&t, "api", &val) == 0);
    val = (void*)3;
    fail_unless(radix_insert(&t, "site", &val) == 0);
    val = (void*)4;
    fail_unless(radix_insert(&t, "api.foo", &val) == 0);
    val = (void*)5;
    fail_unless(radix_insert(&t, "site.bar", &val) == 0);
    val = (void*)6;
    fail_unless(radix_insert(&t, "api.foo.zip", &val) == 0);

    fail_unless(radix_longest_prefix(&t, NULL, &val) == 0);
    fail_unless(val == (void*)1);
    fail_unless(radix_longest_prefix(&t, "", &val) == 0);
    fail_unless(val == (void*)1);
    fail_unless(radix_longest_prefix(&t, "api.zoo", &val) == 0);
    fail_unless(val == (void*)2);
    fail_unless(radix_longest_prefix(&t, "site.other", &val) == 0);
    fail_unless(val == (void*)3);
    fail_unless(radix_longest_prefix(&t, "api.foo.here", &val) == 0);
    fail_unless(val == (void*)4);
    fail_unless(radix_longest_prefix(&t, "api.foo", &val) == 0);
    fail_unless(val == (void*)4);
    fail_unless(radix_longest_prefix(&t, "site.bar.sub", &val) == 0);
    fail_unless(val == (void*)5);
    fail_unless(radix_longest_prefix(&t, "site.bar", &val) == 0);
    fail_unless(val == (void*)5);
    fail_unless(radix_longest_prefix(&t, "api.foo.zip.sub.sub.key", &val) == 0);
    fail_unless(val == (void*)6);
    fail_unless(radix_longest_prefix(&t, "api.foo.zip", &val) == 0);
    fail_unless(val == (void*)6);

    fail_unless(radix_destroy(&t) == 0);
}
END_TEST

static int set_saw(void *d, char *key, void *val) {
    int *saw = (int*)d;
    int v = (uintptr_t)val;
    saw[v-1] = 1;
    return 0;
}

START_TEST(test_radix_foreach)
{
    radix_tree t;
    fail_unless(radix_init(&t) == 0);

    void *val = (void*)1;
    fail_unless(radix_insert(&t, NULL, &val) == 0);
    val = (void*)2;
    fail_unless(radix_insert(&t, "api", &val) == 0);
    val = (void*)3;
    fail_unless(radix_insert(&t, "site", &val) == 0);
    val = (void*)4;
    fail_unless(radix_insert(&t, "api.foo", &val) == 0);
    val = (void*)5;
    fail_unless(radix_insert(&t, "site.bar", &val) == 0);
    val = (void*)6;
    fail_unless(radix_insert(&t, "api.foo.zip", &val) == 0);

    int saw[6];
    fail_unless(radix_foreach(&t, &saw, set_saw) == 0);
    for (int i=0; i<6; i++) {
        fail_unless(saw[i]);
    }

    fail_unless(radix_destroy(&t) == 0);
}
END_TEST

