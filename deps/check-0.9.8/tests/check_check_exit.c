#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "check.h"
#include "check_check.h"

START_TEST(test_early_exit_normal)
{
	exit(0);
	fail("Should've exitted...");
}
END_TEST

START_TEST(test_early_exit_with_allowed_error)
{
	exit(-1);
	fail("Should've exitted...");
}
END_TEST

START_TEST(loop_early_exit_normal)
{
	exit(0);
	fail("Should've exitted...");
}
END_TEST

START_TEST(loop_early_exit_allowed_exit)
{
	exit(-2);
	fail("Should've exitted...");
}
END_TEST

Suite *make_exit_suite(void)
{
  Suite *s;
  TCase *tc;

  s = suite_create("Exit");
  tc = tcase_create("Core");

  suite_add_tcase (s, tc);
  tcase_add_test(tc,test_early_exit_normal);
  tcase_add_exit_test(tc,test_early_exit_with_allowed_error,-1);
  tcase_add_loop_test(tc,loop_early_exit_normal,0,5);
  tcase_add_loop_exit_test(tc,loop_early_exit_allowed_exit,-2,0,5);
  return s;
}
