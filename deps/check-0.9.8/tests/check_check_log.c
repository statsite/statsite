#include "../lib/libcompat.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>
#include "check_check.h"


START_TEST(test_set_log)
{
  Suite *s = suite_create("Suite");
  SRunner *sr = srunner_create(s);

  srunner_set_log (sr, "test_log");

  fail_unless (srunner_has_log (sr), "SRunner not logging");
  fail_unless (strcmp(srunner_log_fname(sr), "test_log") == 0,
	       "Bad file name returned");
}
END_TEST

START_TEST(test_no_set_log)
{
  Suite *s = suite_create("Suite");
  SRunner *sr = srunner_create(s);

  fail_unless (!srunner_has_log (sr), "SRunner not logging");
  fail_unless (srunner_log_fname(sr) == NULL, "Bad file name returned");
}
END_TEST

START_TEST(test_double_set_log)
{
  Suite *s = suite_create("Suite");
  SRunner *sr = srunner_create(s);

  srunner_set_log (sr, "test_log");
  srunner_set_log (sr, "test2_log");

  fail_unless(strcmp(srunner_log_fname(sr), "test_log") == 0,
	      "Log file is initialize only and shouldn't be changeable once set");
}
END_TEST


START_TEST(test_set_xml)
{
  Suite *s = suite_create("Suite");
  SRunner *sr = srunner_create(s);

  srunner_set_xml (sr, "test_log.xml");

  fail_unless (srunner_has_xml (sr), "SRunner not logging XML");
  fail_unless (strcmp(srunner_xml_fname(sr), "test_log.xml") == 0,
	       "Bad file name returned");
}
END_TEST

START_TEST(test_no_set_xml)
{
  Suite *s = suite_create("Suite");
  SRunner *sr = srunner_create(s);

  fail_unless (!srunner_has_xml (sr), "SRunner not logging XML");
  fail_unless (srunner_xml_fname(sr) == NULL, "Bad file name returned");
}
END_TEST

START_TEST(test_double_set_xml)
{
  Suite *s = suite_create("Suite");
  SRunner *sr = srunner_create(s);

  srunner_set_xml (sr, "test_log.xml");
  srunner_set_xml (sr, "test2_log.xml");

  fail_unless(strcmp(srunner_xml_fname(sr), "test_log.xml") == 0,
	      "XML Log file is initialize only and shouldn't be changeable once set");
}
END_TEST

Suite *make_log_suite(void)
{

  Suite *s;
  TCase *tc_core, *tc_core_xml;

  s = suite_create("Log");
  tc_core = tcase_create("Core");
  tc_core_xml = tcase_create("Core XML");

  suite_add_tcase(s, tc_core);
  tcase_add_test(tc_core, test_set_log);
  tcase_add_test(tc_core, test_no_set_log);
  tcase_add_test(tc_core, test_double_set_log);

  suite_add_tcase(s, tc_core_xml);
  tcase_add_test(tc_core_xml, test_set_xml);
  tcase_add_test(tc_core_xml, test_no_set_xml);
  tcase_add_test(tc_core_xml, test_double_set_xml);

  return s;
}

