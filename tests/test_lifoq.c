#include <check.h>
#include "lifoq.h"

START_TEST(test_lifoq_make)
{
    lifoq* q = NULL;
    int ret = lifoq_new(&q, 10);
    fail_unless(q != NULL);
    fail_unless(ret == 0);
}
END_TEST

START_TEST(test_lifoq_basic)
{
    lifoq* q = NULL;
    int* data = malloc(sizeof(int));
    *data = 12;

    lifoq_new(&q, 10);
    int ret = lifoq_push(q, data, 1, true, false);
    fail_unless(ret == 0);

    int* fdata = NULL;
    size_t s = 0;
    ret = lifoq_get(q, (void**)&fdata, &s);
    fail_unless(ret == 0);
    fail_unless(*fdata == 12);
    fail_unless(s == 1);

    free(data);
    free(q);
}
END_TEST

START_TEST(test_lifoq_overflow)
{
    lifoq* q = NULL;
    int* data = malloc(sizeof(int));
    *data = 12;

    lifoq_new(&q, 10);
    int ret = lifoq_push(q, data, 10, true, false);
    fail_unless(ret == 0);

    int* data2 = malloc(sizeof(int));
    *data2 = 13;
    ret = lifoq_push(q, data2, 9, true, false);
    fail_unless(ret == 0);
    /*
     * At this point, data should have been removed from the queue and the
     * queue size adjusted
     */
    int* fdata = NULL;
    size_t s = 0;
    ret = lifoq_get(q, (void**)&fdata, &s);
    fail_unless(ret == 0);
    fail_unless(*fdata == 13);
    fail_unless(s == 9);

    /* Queue should be empty at this state */
    /* Insert a "new" element to confirm it is */
    *data2 = 14;
    ret = lifoq_push(q, data2, 8, true, false);
    fail_unless(ret == 0);
    /* Check that this new element is added */
    ret = lifoq_get(q, (void**)&fdata, &s);
    fail_unless(ret == 0);
    fail_unless(*fdata == 14);
    fail_unless(s == 8);

    /* Do a push with failure */
    ret = lifoq_push(q, data2, 8, true, true);
    fail_unless(ret == 0);
    ret = lifoq_push(q, data2, 8, true, true);
    fail_unless(ret == LIFOQ_FULL);

    free(data2);
    free(q);
}
END_TEST

START_TEST(test_lifoq_close)
{
    lifoq* q = NULL;
    int* data = malloc(sizeof(int));
    *data = 12;

    lifoq_new(&q, 10);
    int ret = lifoq_push(q, data, 10, true, false);
    fail_unless(ret == 0);

    ret = lifoq_close(q);
    fail_unless(ret == 0);

    int* data2 = malloc(sizeof(int));
    *data2 = 13;
    ret = lifoq_push(q, data2, 9, true, false);
    fail_unless(ret == LIFOQ_CLOSED);

    int* fdata = NULL;
    size_t s = 0;
    ret = lifoq_get(q, (void**)&fdata, &s);
    fail_unless(ret == 0);
    fail_unless(*fdata == 12);
    fail_unless(s == 10);

    ret = lifoq_get(q, (void**)&fdata, &s);
    fail_unless(ret == LIFOQ_CLOSED);

    ret = lifoq_close(q);
    fail_unless(ret == LIFOQ_ALREADY_CLOSED);

    free(data2);
    free(data);
    free(q);
}
END_TEST
