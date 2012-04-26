#include <check.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "heap.h"

START_TEST(test_heap_init_and_destroy)
{
    heap h;
    heap_create(&h, 0, NULL);
    fail_unless(heap_size(&h) == 0);
    heap_destroy(&h);
}
END_TEST

START_TEST(test_heap_insert)
{
    heap h;
    heap_create(&h, 0, NULL);
    fail_unless(heap_size(&h) == 0);

    int i = 10;
    heap_insert(&h, &i, NULL);
    fail_unless(heap_size(&h) == 1);

    int *val;
    int res = heap_min(&h, (void**)&val, NULL);
    fail_unless(res == 1);
    fail_unless(*val == i);

    fail_unless(heap_size(&h) == 1);
    heap_destroy(&h);
}
END_TEST

START_TEST(test_heap_insert_delete)
{
    heap h;
    heap_create(&h, 0, NULL);
    fail_unless(heap_size(&h) == 0);

    int i = 10;
    heap_insert(&h, &i, (void*)100);
    fail_unless(heap_size(&h) == 1);

    int *val;
    void *other;
    int res = heap_delmin(&h, (void**)&val, (void**)&other);
    fail_unless(res == 1);
    fail_unless(*val == i);
    fail_unless(other == (void*)100);
    fail_unless(heap_size(&h) == 0);

    heap_destroy(&h);
}
END_TEST

START_TEST(test_heap_delete_order)
{
    heap h;
    heap_create(&h, 0, NULL);
    fail_unless(heap_size(&h) == 0);

    int *keys = alloca(10*sizeof(int));
    for (int i=10; i>0; i--) {
        keys[i-1] = i;
        heap_insert(&h, keys+(i-1), NULL);
    }
    fail_unless(heap_size(&h) == 10);

    int *val;
    fail_unless(heap_min(&h, (void**)&val, NULL) == 1);
    fail_unless(*val == 1);

    for (int i=1; i<=10; i++) {
        heap_delmin(&h, (void**)&val, NULL);
        fail_unless(*val == i);
    }
    fail_unless(heap_size(&h) == 0);

    heap_destroy(&h);
}
END_TEST

void for_each_cb(void *key, void* val) {
    int *n = key;
    int *v = val;
    *v = *n;
}

START_TEST(test_heap_for_each)
{
    heap h;
    heap_create(&h, 0, NULL);
    fail_unless(heap_size(&h) == 0);

    int *keys = alloca(10*sizeof(int));
    int *vals= alloca(10*sizeof(int));
    for (int i=10; i>0; i--) {
        keys[i-1] = i;
        vals[i-1] = 0;
        heap_insert(&h, keys+(i-1), vals+(i-1));
    }
    fail_unless(heap_size(&h) == 10);

    // Callback each
    heap_foreach(&h, for_each_cb);

    // Check everything
    for (int i=1; i<=10; i++) {
        fail_unless(vals[i-1] == i);
    }

    heap_destroy(&h);
}
END_TEST

START_TEST(test_heap_del_empty)
{
    heap h;
    heap_create(&h, 0, NULL);
    fail_unless(heap_size(&h) == 0);

    void *key=NULL, *val=NULL;
    fail_unless(heap_min(&h, &key, &val) == 0);

    fail_unless(heap_delmin(&h, &key, &val) == 0);
    fail_unless(key == NULL);
    fail_unless(val == NULL);

    heap_destroy(&h);
}
END_TEST

