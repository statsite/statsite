#include <stdlib.h>
#include <check.h>
#include "../src/money.h"

Money *five_dollars;

void
setup (void)
{
  five_dollars = money_create (5, "USD");
}

void
teardown (void)
{
  money_free (five_dollars);
}

START_TEST (test_money_create)
{
  fail_unless (money_amount (five_dollars) == 5,
	       "Amount not set correctly on creation");
  fail_unless (strcmp (money_currency (five_dollars), "USD") == 0,
	       "Currency not set correctly on creation");
}
END_TEST

START_TEST (test_money_create_neg)
{
  Money *m = money_create (-1, "USD");
  fail_unless (m == NULL,
	       "NULL should be returned on attempt to create with "
	       "a negative amount");
}
END_TEST

START_TEST (test_money_create_zero)
{
  Money *m = money_create (0, "USD");
  fail_unless (money_amount (m) == 0, 
	       "Zero is a valid amount of money");
}
END_TEST

Suite *
money_suite (void)
{
  Suite *s = suite_create ("Money");

  /* Core test case */
  TCase *tc_core = tcase_create ("Core");
  tcase_add_checked_fixture (tc_core, setup, teardown);
  tcase_add_test (tc_core, test_money_create);
  suite_add_tcase (s, tc_core);

  /* Limits test case */
  TCase *tc_limits = tcase_create ("Limits");
  tcase_add_test (tc_limits, test_money_create_neg);
  tcase_add_test (tc_limits, test_money_create_zero);
  suite_add_tcase (s, tc_limits);

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
