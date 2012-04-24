#include "../lib/libcompat.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <check.h>

START_TEST(test_pass)
{
  fail_unless (1==1, "Shouldn't see this");
}
END_TEST

START_TEST(test_fail)
{
  fail("Failure");
}
END_TEST

START_TEST(test_exit)
{
  exit(1);
}
END_TEST

START_TEST(test_pass2)
{
  fail_unless (1==1, "Shouldn't see this");
}
END_TEST

START_TEST(test_loop)
{
  fail_unless (_i==1, "Iteration %d failed", _i);
}
END_TEST

static Suite *make_s1_suite (void)
{
  Suite *s;
  TCase *tc;

  s = suite_create("S1");
  tc = tcase_create ("Core");
  suite_add_tcase(s, tc);
  tcase_add_test (tc, test_pass);
  tcase_add_test (tc, test_fail);
  tcase_add_test (tc, test_exit);

  return s;
}

static Suite *make_s2_suite (void)
{
  Suite *s;
  TCase *tc;

  s = suite_create("S2");
  tc = tcase_create ("Core");
  suite_add_tcase(s, tc);
  tcase_add_test (tc, test_pass2);
  tcase_add_loop_test(tc, test_loop, 0, 3);

  return s;
}

static void run_tests (int printmode)
{
  SRunner *sr;

  sr = srunner_create(make_s1_suite());
  srunner_add_suite(sr, make_s2_suite());
  srunner_set_xml(sr, "test.log.xml");
  srunner_run_all(sr, printmode);
  srunner_free(sr);
}


int main (int argc CK_ATTRIBUTE_UNUSED, char **argv CK_ATTRIBUTE_UNUSED)
{
  run_tests(CK_SILENT);		/* not considered in XML output */

  return EXIT_SUCCESS;
}
  
