/**
 * The purpose of this test is to be used by valgrind to check for
 * memory leaks. Each public API that check exports is used at
 * least once. Tests which use non-public API, or leak intentionally,
 * are not included here.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <check.h>
#include "config.h"
#include "check_check.h"

int main ()
{
    int n;
    SRunner *sr;

    /*
     * First, the sub suite is run. This suite has failures which
     * are intentional, as the output of the failures is checked
     * in check_check_master.c. However, here we do not check if
     * the failures are expected. Instead, we just want to run
     * them and see if they leak. Because of this, the result
     * of the suite is not checked.
     */
    sr = srunner_create(make_sub_suite());
    /*
     * Enable all logging types, just in case one of them
     * leaks memory.
     */
    srunner_set_log (sr, "test_mem_leak.log");
    srunner_set_xml (sr, "test_mem_leak.xml");
    srunner_set_tap (sr, "test_mem_leak.tap");
    srunner_run_all(sr, CK_NORMAL);
    srunner_free(sr);

    /* Now, the other suite is run. These are all expected to pass. */

    /* The following setup is necessary for the fork suite */
    fork_setup();

    sr = srunner_create (make_log_suite());
    srunner_add_suite(sr, make_fork_suite());

#if defined(HAVE_FORK) && HAVE_FORK==1
    srunner_add_suite(sr, make_exit_suite());
#endif

    srunner_add_suite(sr, make_selective_suite());

    /*
     * Enable all logging types, just in case one of them
     * leaks memory.
     */
    srunner_set_log (sr, "test_mem_leak.log");
    srunner_set_xml (sr, "test_mem_leak.xml");
    srunner_set_tap (sr, "test_mem_leak.tap");

    srunner_run_all(sr, CK_NORMAL);

    /* Cleanup from the fork suite setup */
    fork_teardown();

    n = srunner_ntests_failed(sr);
    srunner_free(sr);
    return (n == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

