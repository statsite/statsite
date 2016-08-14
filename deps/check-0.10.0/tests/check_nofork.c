#include "../lib/libcompat.h"

#include <stdlib.h>
#include <stdio.h>
#include <check.h>


Suite *s;
TCase *tc;
SRunner *sr;

START_TEST(test_nofork_exit)
{
  char* s = NULL;

  ck_assert(NULL != s);

  /* this test should not crash in nofork mode */
  ck_assert_str_eq("test", s);
}
END_TEST

#if !defined(HAVE_FORK) || HAVE_FORK == 0
START_TEST(test_check_fork)
{
    ck_assert_int_eq(-1, check_fork());
}
END_TEST
#endif

int main(void)
{
  s = suite_create("NoFork");
  tc = tcase_create("Exit");
  sr = srunner_create(s);

  suite_add_tcase(s, tc);
  tcase_add_test(tc, test_nofork_exit);

  srunner_set_fork_status(sr, CK_NOFORK);
  srunner_run_all(sr, CK_MINIMAL);
  srunner_free(sr);

#if !defined(HAVE_FORK) || HAVE_FORK == 0
  s = suite_create("NoForkSupport");
  tc = tcase_create("NoFork");
  sr = srunner_create(s);

  /* The following should not fail, but should be ignored */
  srunner_set_fork_status(sr, CK_FORK);
  if(srunner_fork_status(sr) != CK_NOFORK)
  {
      fprintf(stderr, "Call to srunner_set_fork_status() was not ignored\n");
      exit(1);
  }

  suite_add_tcase(s, tc);
  tcase_add_test(tc, test_check_fork);
  srunner_run_all(sr, CK_MINIMAL);
  srunner_free(sr);
#endif

  return 0;
}
