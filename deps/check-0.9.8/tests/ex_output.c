#include "../lib/libcompat.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <check.h>
#include "config.h"

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

static Suite *make_suite (void)
{
  Suite *s;
  TCase *tc;

  s = suite_create("Master");
  tc = tcase_create ("Core");
  suite_add_tcase(s, tc);
  tcase_add_test (tc, test_pass);
  tcase_add_test (tc, test_fail);
  tcase_add_test (tc, test_exit);

  return s;
}

static void run_tests (int printmode)
{
  SRunner *sr;
  Suite *s;

  s = make_suite();
  sr = srunner_create(s);
  srunner_run_all(sr, printmode);
  srunner_free(sr);
}

static void print_usage(void)
{
    printf ("Usage: ex_output (CK_SILENT | CK_MINIMAL | CK_NORMAL | CK_VERBOSE");
#if ENABLE_SUBUNIT
    printf (" | CK_SUBUNIT");
#endif
    printf (")\n");
}

int main (int argc, char **argv)
{
  
  if (argc != 2) {
    print_usage();
    return EXIT_FAILURE;
  }

  if (strcmp (argv[1], "CK_SILENT") == 0)
    run_tests(CK_SILENT);
  else if (strcmp (argv[1], "CK_MINIMAL") == 0)
    run_tests(CK_MINIMAL);
  else if (strcmp (argv[1], "CK_NORMAL") == 0)
    run_tests(CK_NORMAL);
  else if (strcmp (argv[1], "CK_VERBOSE") == 0)
    run_tests(CK_VERBOSE);
#if ENABLE_SUBUNIT
  else if (strcmp (argv[1], "CK_SUBUNIT") == 0)
    run_tests(CK_SUBUNIT);
#endif
  else {
    print_usage();
    return EXIT_FAILURE;
  }    
    

  return EXIT_SUCCESS;
}
  
