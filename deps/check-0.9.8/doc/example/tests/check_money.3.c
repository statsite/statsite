#include <stdlib.h>
#include <check.h>
#include "../src/money.h"

START_TEST (test_money_create)
{
  Money *m;
  m = money_create (5, "USD");
  fail_unless (money_amount (m) == 5, 
	       "Amount not set correctly on creation");
  fail_unless (strcmp (money_currency (m), "USD") == 0,
	       "Currency not set correctly on creation");
  money_free (m);
}
END_TEST

Suite *
money_suite (void)
{
  Suite *s = suite_create ("Money");

  /* Core test case */
  TCase *tc_core = tcase_create ("Core");
  tcase_add_test (tc_core, test_money_create);
  suite_add_tcase (s, tc_core);

  return s;
}

int
main (void)
{
  int number_failed;
  Suite *s = money_suite ();
  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
