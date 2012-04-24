#include <check.h>
#include <stdio.h>
#include <syslog.h>
#include "test_config.c"
#include "test_hashmap.c"
#include "test_filter.c"
#include "test_filtmgr.c"

int main(void)
{
    setlogmask(LOG_UPTO(LOG_DEBUG));

    Suite *s1 = suite_create("Bloomd");
    TCase *tc1 = tcase_create("config");
    TCase *tc2 = tcase_create("hashmap");
    TCase *tc3 = tcase_create("filter");
    TCase *tc4 = tcase_create("filter manager");
    SRunner *sr = srunner_create(s1);
    int nf;

    // Add the config tests
    suite_add_tcase(s1, tc1);
    tcase_add_test(tc1, test_config_get_default);
    tcase_add_test(tc1, test_config_bad_file);
    tcase_add_test(tc1, test_config_empty_file);
    tcase_add_test(tc1, test_config_basic_config);
    tcase_add_test(tc1, test_validate_default_config);
    tcase_add_test(tc1, test_validate_bad_config);
    tcase_add_test(tc1, test_join_path_no_slash);
    tcase_add_test(tc1, test_join_path_with_slash);
    tcase_add_test(tc1, test_sane_log_level);
    tcase_add_test(tc1, test_sane_init_capacity);
    tcase_add_test(tc1, test_sane_default_probability);
    tcase_add_test(tc1, test_sane_scale_size);
    tcase_add_test(tc1, test_sane_probability_reduction);
    tcase_add_test(tc1, test_sane_flush_interval);
    tcase_add_test(tc1, test_sane_cold_interval);
    tcase_add_test(tc1, test_sane_in_memory);
    tcase_add_test(tc1, test_sane_worker_threads);
    tcase_add_test(tc1, test_filter_config_bad_file);
    tcase_add_test(tc1, test_filter_config_empty_file);
    tcase_add_test(tc1, test_filter_config_basic_config);
    tcase_add_test(tc1, test_update_filename_from_filter_config);

    // Add the hashmap tests
    suite_add_tcase(s1, tc2);
    tcase_add_test(tc2, test_map_init_and_destroy);
    tcase_add_test(tc2, test_map_get_no_keys);
    tcase_add_test(tc2, test_map_put);
    tcase_add_test(tc2, test_map_put_get);
    tcase_add_test(tc2, test_map_delete_no_keys);
    tcase_add_test(tc2, test_map_put_delete);
    tcase_add_test(tc2, test_map_put_delete_get);
    tcase_add_test(tc2, test_map_clear_no_keys);
    tcase_add_test(tc2, test_map_put_clear_get);
    tcase_add_test(tc2, test_map_iter_no_keys);
    tcase_add_test(tc2, test_map_put_iter_break);
    tcase_add_test(tc2, test_map_put_grow);

    // Add the filter tests
    suite_add_tcase(s1, tc3);
    tcase_set_timeout(tc3, 3);
    tcase_add_test(tc3, test_filter_init_destroy);
    tcase_add_test(tc3, test_filter_init_discover_destroy);
    tcase_add_test(tc3, test_filter_init_discover_delete);
    tcase_add_test(tc3, test_filter_init_proxied);
    tcase_add_test(tc3, test_filter_add_check);
    tcase_add_test(tc3, test_filter_restore);
    tcase_add_test(tc3, test_filter_flush);
    tcase_add_test(tc3, test_filter_add_check_in_mem);
    tcase_add_test(tc3, test_filter_grow);
    tcase_add_test(tc3, test_filter_grow_restore);
    tcase_add_test(tc3, test_filter_restore_order);
    tcase_add_test(tc3, test_filter_page_out);
    tcase_add_test(tc3, test_filter_bounded_fp);

    // Add the filter tests
    suite_add_tcase(s1, tc4);
    tcase_set_timeout(tc4, 3);
    tcase_add_test(tc4, test_mgr_init_destroy);
    tcase_add_test(tc4, test_mgr_create_drop);
    tcase_add_test(tc4, test_mgr_create_double_drop);
    tcase_add_test(tc4, test_mgr_list);
    tcase_add_test(tc4, test_mgr_list_no_filters);
    tcase_add_test(tc4, test_mgr_add_check_keys);
    tcase_add_test(tc4, test_mgr_check_no_keys);
    tcase_add_test(tc4, test_mgr_add_check_no_filter);
    tcase_add_test(tc4, test_mgr_flush_no_filter);
    tcase_add_test(tc4, test_mgr_flush);
    tcase_add_test(tc4, test_mgr_unmap_no_filter);
    tcase_add_test(tc4, test_mgr_unmap);
    tcase_add_test(tc4, test_mgr_unmap_add_keys);
    tcase_add_test(tc4, test_mgr_list_cold_no_filters);
    tcase_add_test(tc4, test_mgr_list_cold);
    tcase_add_test(tc4, test_mgr_unmap_in_mem);
    tcase_add_test(tc4, test_mgr_create_custom_config);
    tcase_add_test(tc4, test_mgr_grow);
    tcase_add_test(tc4, test_mgr_restore);
    tcase_add_test(tc4, test_mgr_callback);

    srunner_run_all(sr, CK_ENV);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);

    return nf == 0 ? 0 : 1;
}

