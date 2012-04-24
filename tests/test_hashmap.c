#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "hashmap.h"

START_TEST(test_map_init_and_destroy)
{
    bloom_hashmap *map;
    int res = hashmap_init(0, &map);
    fail_unless(res == 0);
    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_map_get_no_keys)
{
    bloom_hashmap *map;
    int res = hashmap_init(0, &map);
    fail_unless(res == 0);

    fail_unless(hashmap_size(map) == 0);

    char buf[100];
    void *out;
    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        fail_unless(hashmap_get(map, (char*)buf, &out) == -1);
    }

    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_map_put)
{
    bloom_hashmap *map;
    int res = hashmap_init(0, &map);
    fail_unless(res == 0);
    fail_unless(hashmap_size(map) == 0);

    char buf[100];
    void *out;
    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        fail_unless(hashmap_put(map, (char*)buf, NULL) == 1);
    }
    fail_unless(hashmap_size(map) == 100);

    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_map_put_get)
{
    bloom_hashmap *map;
    int res = hashmap_init(0, &map);
    fail_unless(res == 0);

    char buf[100];
    void *out;
    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        out = 0 & i;
        fail_unless(hashmap_put(map, (char*)buf, out) == 1);
    }

    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        fail_unless(hashmap_get(map, (char*)buf, &out) == 0);
        fail_unless(out == (0 & i));
    }

    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_map_delete_no_keys)
{
    bloom_hashmap *map;
    int res = hashmap_init(0, &map);
    fail_unless(res == 0);
    fail_unless(hashmap_size(map) == 0);

    char buf[100];
    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        fail_unless(hashmap_delete(map, (char*)buf) == -1);
    }

    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_map_put_delete)
{
    bloom_hashmap *map;
    int res = hashmap_init(0, &map);
    fail_unless(res == 0);

    char buf[100];
    void *out;
    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        out = 0 & i;
        fail_unless(hashmap_put(map, (char*)buf, out) == 1);
    }
    fail_unless(hashmap_size(map) == 100);

    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        fail_unless(hashmap_delete(map, (char*)buf) == 0);
        fail_unless(hashmap_size(map) == (100-i-1));
    }

    fail_unless(hashmap_size(map) == 0);
    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_map_put_delete_get)
{
    bloom_hashmap *map;
    int res = hashmap_init(0, &map);
    fail_unless(res == 0);

    char buf[100];
    void *out;
    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        out = 0 & i;
        fail_unless(hashmap_put(map, (char*)buf, out) == 1);
    }

    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        fail_unless(hashmap_delete(map, (char*)buf) == 0);
    }

    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        fail_unless(hashmap_get(map, (char*)buf, &out) == -1);
    }

    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_map_clear_no_keys)
{
    bloom_hashmap *map;
    int res = hashmap_init(0, &map);
    fail_unless(res == 0);

    fail_unless(hashmap_clear(map) == 0);

    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_map_put_clear_get)
{
    bloom_hashmap *map;
    int res = hashmap_init(0, &map);
    fail_unless(res == 0);

    char buf[100];
    void *out;
    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        out = 0 & i;
        fail_unless(hashmap_put(map, (char*)buf, out) == 1);
    }

    fail_unless(hashmap_size(map) == 100);
    fail_unless(hashmap_clear(map) == 0);
    fail_unless(hashmap_size(map) == 0);

    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        fail_unless(hashmap_get(map, (char*)buf, &out) == -1);
    }

    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

int iter_test(void *data, const char *key, void *value) {
    // All we do is increment val
    int *v = data;
    *v += 1;
    return 0;
}

START_TEST(test_map_iter_no_keys)
{
    bloom_hashmap *map;
    int res = hashmap_init(0, &map);
    fail_unless(res == 0);

    int val = 0;
    fail_unless(hashmap_iter(map, iter_test, (void*)&val) == 0);
    fail_unless(val == 0);

    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_map_put_iter)
{
    bloom_hashmap *map;
    int res = hashmap_init(0, &map);
    fail_unless(res == 0);

    char buf[100];
    void *out;
    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        fail_unless(hashmap_put(map, (char*)buf, NULL) == 1);
    }

    int val = 0;
    fail_unless(hashmap_iter(map, iter_test, (void*)&val) == 0);
    fail_unless(val == 100);

    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

int iter_break_test(void *data, const char *key, void *value) {
    // All we do is increment val
    int *v = data;
    *v += 1;
    return 1; // SHOULD BREAK
}

START_TEST(test_map_put_iter_break)
{
    bloom_hashmap *map;
    int res = hashmap_init(0, &map);
    fail_unless(res == 0);

    char buf[100];
    void *out;
    for (int i=0; i<100;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        fail_unless(hashmap_put(map, (char*)buf, NULL) == 1);
    }

    int val = 0;
    fail_unless(hashmap_iter(map, iter_break_test, (void*)&val) == 0);
    fail_unless(val == 1);

    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

START_TEST(test_map_put_grow)
{
    bloom_hashmap *map;
    int res = hashmap_init(32, &map);  // Only 32 slots
    fail_unless(res == 0);

    char buf[100];
    void *out;
    for (int i=0; i<1000;i++) {
        snprintf((char*)&buf, 100, "test%d", i);
        fail_unless(hashmap_put(map, (char*)buf, NULL) == 1);
    }

    int val = 0;
    fail_unless(hashmap_iter(map, iter_test, (void*)&val) == 0);
    fail_unless(val == 1000);
    fail_unless(hashmap_size(map) == 1000);

    res = hashmap_destroy(map);
    fail_unless(res == 0);
}
END_TEST

