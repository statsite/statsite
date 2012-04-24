#include <check.h>
#include <stdio.h>
#include <syslog.h>
#include "test_hashmap.c"

int main(void)
{
    setlogmask(LOG_UPTO(LOG_DEBUG));

    Suite *s1 = suite_create("Statsite");
    TCase *tc1 = tcase_create("hashmap");
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

    srunner_run_all(sr, CK_ENV);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);

    return nf == 0 ? 0 : 1;
}

