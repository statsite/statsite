#include <check.h>
#include <stdio.h>
#include <syslog.h>
#include "test_hashmap.c"
#include "test_cm_quantile.c"
#include "test_heap.c"
#include "test_timer.c"
#include "test_counter.c"
#include "test_metrics.c"
#include "test_streaming.c"

int main(void)
{
    setlogmask(LOG_UPTO(LOG_DEBUG));

    Suite *s1 = suite_create("Statsite");
    TCase *tc1 = tcase_create("hashmap");
    TCase *tc2 = tcase_create("quantile");
    TCase *tc3 = tcase_create("heap");
    TCase *tc4 = tcase_create("timer");
    TCase *tc5 = tcase_create("counter");
    TCase *tc6 = tcase_create("metrics");
    TCase *tc7 = tcase_create("streaming");
    SRunner *sr = srunner_create(s1);
    int nf;

    // Add the hashmap tests
    suite_add_tcase(s1, tc1);
    tcase_add_test(tc1, test_map_init_and_destroy);
    tcase_add_test(tc1, test_map_get_no_keys);
    tcase_add_test(tc1, test_map_put);
    tcase_add_test(tc1, test_map_put_get);
    tcase_add_test(tc1, test_map_delete_no_keys);
    tcase_add_test(tc1, test_map_put_delete);
    tcase_add_test(tc1, test_map_put_delete_get);
    tcase_add_test(tc1, test_map_clear_no_keys);
    tcase_add_test(tc1, test_map_put_clear_get);
    tcase_add_test(tc1, test_map_iter_no_keys);
    tcase_add_test(tc1, test_map_put_iter_break);
    tcase_add_test(tc1, test_map_put_grow);

    // Add the quantile tests
    suite_add_tcase(s1, tc2);
    tcase_add_test(tc2, test_cm_init_and_destroy);
    tcase_add_test(tc2, test_cm_init_no_quants);
    tcase_add_test(tc2, test_cm_init_bad_quants);
    tcase_add_test(tc2, test_cm_init_bad_eps);
    tcase_add_test(tc2, test_cm_init_add_destroy);
    tcase_add_test(tc2, test_cm_init_add_loop_destroy);
    tcase_add_test(tc2, test_cm_init_query_destroy);
    tcase_add_test(tc2, test_cm_init_add_query_destroy);
    tcase_add_test(tc2, test_cm_init_add_loop_query_destroy);
    tcase_add_test(tc2, test_cm_init_add_loop_rev_query_destroy);
    tcase_add_test(tc2, test_cm_init_add_loop_random_query_destroy);

    // Add the heap tests
    suite_add_tcase(s1, tc3);
    tcase_add_test(tc3, test_heap_init_and_destroy);
    tcase_add_test(tc3, test_heap_insert);
    tcase_add_test(tc3, test_heap_insert_delete);
    tcase_add_test(tc3, test_heap_delete_order);
    tcase_add_test(tc3, test_heap_for_each);
    tcase_add_test(tc3, test_heap_del_empty);

    // Add the timer tests
    suite_add_tcase(s1, tc4);
    tcase_add_test(tc4, test_timer_init_and_destroy);
    tcase_add_test(tc4, test_timer_init_add_destroy);
    tcase_add_test(tc4, test_timer_add_loop);

    // Add the counter tests
    suite_add_tcase(s1, tc5);
    tcase_add_test(tc5, test_counter_init);
    tcase_add_test(tc5, test_counter_init_add);
    tcase_add_test(tc5, test_counter_add_loop);

    // Add the counter tests
    suite_add_tcase(s1, tc6);
    tcase_add_test(tc6, test_metrics_init_and_destroy);
    tcase_add_test(tc6, test_metrics_init_defaults_and_destroy);
    tcase_add_test(tc6, test_metrics_empty_iter);
    tcase_add_test(tc6, test_metrics_add_iter);
    tcase_add_test(tc6, test_metrics_add_all_iter);

    // Add the streaming tests
    suite_add_tcase(s1, tc7);
    tcase_add_test(tc7, test_stream_empty);
    tcase_add_test(tc7, test_stream_some);
    tcase_add_test(tc7, test_stream_bad_cmd);
    tcase_add_test(tc7, test_stream_sigpipe);

    srunner_run_all(sr, CK_ENV);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);

    return nf == 0 ? 0 : 1;
}

