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
#include "test_config.c"
#include "test_radix.c"
#include "test_hll.c"
#include "test_set.c"

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
    TCase *tc8 = tcase_create("config");
    TCase *tc9 = tcase_create("radix");
    TCase *tc10 = tcase_create("hyperloglog");
    TCase *tc11 = tcase_create("set");
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
    tcase_add_test(tc2, test_cm_init_add3_query_destroy);
    tcase_add_test(tc2, test_cm_init_add_negative_query_destroy);
    tcase_add_test(tc2, test_cm_init_add_loop_query_destroy);
    tcase_add_test(tc2, test_cm_init_add_loop_tail_query_destroy);
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
    tcase_add_test(tc4, test_timer_sample_rate);

    // Add the counter tests
    suite_add_tcase(s1, tc5);
    tcase_add_test(tc5, test_counter_init);
    tcase_add_test(tc5, test_counter_init_add);
    tcase_add_test(tc5, test_counter_add_loop);
    tcase_add_test(tc5, test_counter_sample_rate);

    // Add the counter tests
    suite_add_tcase(s1, tc6);
    tcase_add_test(tc6, test_metrics_init_and_destroy);
    tcase_add_test(tc6, test_metrics_init_defaults_and_destroy);
    tcase_add_test(tc6, test_metrics_empty_iter);
    tcase_add_test(tc6, test_metrics_add_iter);
    tcase_add_test(tc6, test_metrics_add_all_iter);
    tcase_add_test(tc6, test_metrics_histogram);
    tcase_add_test(tc6, test_metrics_gauges);

    // Add the streaming tests
    suite_add_tcase(s1, tc7);
    tcase_add_test(tc7, test_stream_empty);
    tcase_add_test(tc7, test_stream_some);
    tcase_add_test(tc7, test_stream_bad_cmd);
    tcase_add_test(tc7, test_stream_sigpipe);

    // Add the config tests
    suite_add_tcase(s1, tc8);
    tcase_add_test(tc8, test_config_get_default);
    tcase_add_test(tc8, test_config_bad_file);
    tcase_add_test(tc8, test_config_empty_file);
    tcase_add_test(tc8, test_config_basic_config);
    tcase_add_test(tc8, test_validate_default_config);
    tcase_add_test(tc8, test_validate_bad_config);
    tcase_add_test(tc8, test_join_path_no_slash);
    tcase_add_test(tc8, test_join_path_with_slash);
    tcase_add_test(tc8, test_sane_log_level);
    tcase_add_test(tc8, test_sane_log_facility);
    tcase_add_test(tc8, test_sane_timer_eps);
    tcase_add_test(tc8, test_sane_flush_interval);
    tcase_add_test(tc8, test_sane_histograms);
    tcase_add_test(tc8, test_sane_set_eps);
    tcase_add_test(tc8, test_config_histograms);
    tcase_add_test(tc8, test_build_radix);
    tcase_add_test(tc8, test_sane_prefixes);
    tcase_add_test(tc8, test_sane_global_prefix);
    tcase_add_test(tc8, test_sane_quantiles);
    tcase_add_test(tc8, test_extended_counters_include_count_only);
    tcase_add_test(tc8, test_extended_counters_include_count_rate);
    tcase_add_test(tc8, test_extended_counters_include_all_selected);
    tcase_add_test(tc8, test_extended_counters_include_all_by_default);
    tcase_add_test(tc8, test_timers_include_count_only);
    tcase_add_test(tc8, test_timers_include_count_rate);
    tcase_add_test(tc8, test_timers_include_all_selected);

    // Add the radix tests
    suite_add_tcase(s1, tc9);
    tcase_add_test(tc9, test_radix_init_and_destroy);
    tcase_add_test(tc9, test_radix_insert);
    tcase_add_test(tc9, test_radix_search);
    tcase_add_test(tc9, test_radix_longest_prefix);
    tcase_add_test(tc9, test_radix_foreach);

    // Add the hll tests
    suite_add_tcase(s1, tc10);
    tcase_add_test(tc10, test_hll_init_bad);
    tcase_add_test(tc10, test_hll_init_and_destroy);
    tcase_add_test(tc10, test_hll_add);
    tcase_add_test(tc10, test_hll_add_hash);
    tcase_add_test(tc10, test_hll_add_size);
    tcase_add_test(tc10, test_hll_size);
    tcase_add_test(tc10, test_hll_error_bound);
    tcase_add_test(tc10, test_hll_precision_for_error);

    // Add the set tests
    suite_add_tcase(s1, tc11);
    tcase_add_test(tc11, test_set_init_destroy);
    tcase_add_test(tc11, test_set_add_size_exact);
    tcase_add_test(tc11, test_set_add_size_exact_dedup);
    tcase_add_test(tc11, test_set_error_bound);


    srunner_run_all(sr, CK_ENV);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);

    return nf == 0 ? 0 : 1;
}
