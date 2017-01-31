#include "../lib/libcompat.h"

#include <string.h>
#include <stdlib.h>

#include "check.h"
#include "check_list.h"
#include "check_check.h"

START_TEST(test_create)
{
  List *lp = NULL;

  ck_assert_msg (check_list_val(lp) == NULL,
	       "Current list value should be NULL for NULL list");

  lp = check_list_create();

  ck_assert_msg (check_list_val(lp) == NULL,
	       "Current list value should be NULL for newly created list");

  ck_assert_msg (check_list_at_end(lp),
	       "Newly created list should be at end");
  check_list_advance(lp);
  ck_assert_msg (check_list_at_end(lp),
	       "Advancing a list at end should produce a list at end");
  check_list_free (lp);
}
END_TEST

START_TEST(test_free)
{
  List *lp = check_list_create();
  char data_abc[] = "abc";
  char data_123[] = "123";

  check_list_add_end (lp, data_abc);
  check_list_add_end (lp, data_123);
  check_list_add_end (lp, NULL);
  check_list_free (lp);
}
END_TEST

START_TEST(test_add_end)
{
  List * lp = check_list_create();
  char tval[] = "abc";
  
  check_list_add_end (lp, tval);
  
  ck_assert_msg (check_list_val (lp) != NULL,
	       "List current val should not be null after new insertion");
  ck_assert_msg (!check_list_at_end (lp),
	       "List should be at end after new insertion");
  ck_assert_msg (strcmp(tval, (char *) check_list_val (lp)) == 0,
	       "List current val should equal newly inserted val");
  check_list_free (lp);
}
END_TEST

START_TEST(test_add_front)
{
  List * lp = check_list_create();
  char tval[] = "abc";
  
  check_list_add_front (lp, tval);
  
  ck_assert_msg (check_list_val (lp) != NULL,
	       "List current val should not be null after new insertion");
  ck_assert_msg (strcmp(tval, (char *) check_list_val (lp)) == 0,
	       "List current val should equal newly inserted val");
  check_list_free (lp);
}
END_TEST

START_TEST(test_add_end_and_next)
{
  List *lp = check_list_create();
  char tval1[] = "abc";
  char tval2[] = "123";
  
  check_list_add_end (lp, tval1);
  check_list_add_end (lp, tval2);
  check_list_front(lp);
  ck_assert_msg (strcmp (tval1, (char *)check_list_val (lp)) == 0,
	       "List head val should equal first inserted val");
  check_list_advance (lp);
  ck_assert_msg (!check_list_at_end (lp),
	       "List should not be at end after two adds and one next");
  ck_assert_msg (strcmp (tval2, (char *)check_list_val (lp)) == 0,
	       "List val should equal second inserted val");
  check_list_advance(lp);
  ck_assert_msg (check_list_at_end (lp),
	       "List should be at and after two adds and two nexts");
  check_list_free (lp);
}
END_TEST


START_TEST(test_add_front_and_next)
{
  List * lp = check_list_create();
  char tval1[] = "abc";
  char tval2[] = "123";
  
  check_list_add_front (lp, tval1);
  check_list_add_front (lp, tval2);
  check_list_front(lp);
  ck_assert_msg (strcmp (tval2, (char *)check_list_val (lp)) == 0,
	       "List head val should equal last inserted val");
  check_list_advance (lp);
  ck_assert_msg (!check_list_at_end (lp),
	       "List should not be at end after two adds and one next");
  ck_assert_msg (strcmp (tval1, (char *)check_list_val (lp)) == 0,
	       "List val should equal first inserted val");
  check_list_advance(lp);
  ck_assert_msg (check_list_at_end (lp),
	       "List should be at and after two adds and two nexts");
  check_list_free (lp);
}
END_TEST

START_TEST(test_add_a_bunch)
{
  List *lp;
  int i, j;
  char tval1[] = "abc";
  char tval2[] = "123";
  for (i = 0; i < 3; i++) {
    lp = check_list_create();
    for (j = 0; j < 1000; j++) {
      check_list_add_end (lp, tval1);
      check_list_add_front (lp, tval2);
    }
    check_list_free(lp);
  }
}
END_TEST

START_TEST(test_list_abuse)
{
    check_list_advance(NULL);
    /* Should not crash */
}
END_TEST


Suite *make_list_suite (void)
{
  Suite *s = suite_create("Lists");
  TCase * tc = tcase_create("Core");

  suite_add_tcase (s, tc);
  tcase_add_test (tc, test_create);
  tcase_add_test (tc, test_free);
  tcase_add_test (tc, test_add_end);
  tcase_add_test (tc, test_add_front);
  tcase_add_test (tc, test_add_end_and_next);
  tcase_add_test (tc, test_add_front_and_next);
  tcase_add_test (tc, test_add_a_bunch);
  tcase_add_test (tc, test_list_abuse);

  return s;
}
