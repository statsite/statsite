#include "../lib/libcompat.h"

#include <stdlib.h>
#include <stdio.h>
#include <check.h>
#include "check_check.h"

int main (void)
{
  int n;
  SRunner *sr;

#ifdef _POSIX_VERSION
  fork_setup();
  setup_fixture();
  setup();
#endif /* _POSIX_VERSION */

  sr = srunner_create (make_master_suite());
  srunner_add_suite(sr, make_list_suite());
  srunner_add_suite(sr, make_msg_suite());
  srunner_add_suite(sr, make_log_suite());
  srunner_add_suite(sr, make_log_internal_suite());
  srunner_add_suite(sr, make_limit_suite());
  srunner_add_suite(sr, make_fork_suite());
  srunner_add_suite(sr, make_fixture_suite());
  srunner_add_suite(sr, make_pack_suite());
  srunner_add_suite(sr, make_exit_suite());
  
  printf ("Ran %d tests in subordinate suite\n", sub_ntests);
  srunner_run_all (sr, CK_VERBOSE);
  cleanup();
  fork_teardown();
  teardown_fixture();
  n = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (n == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
