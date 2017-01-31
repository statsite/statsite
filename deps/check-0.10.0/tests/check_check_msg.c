#include "../lib/libcompat.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "check.h"
#include "check_msg.h"
#include "check_check.h"
#include "check_list.h"
#include "check_impl.h"

START_TEST(test_send)
{
  TestResult *tr;
  setup_messaging();
  send_ctx_info(CK_CTX_SETUP);
  send_loc_info("abc123.c", 10);
  send_ctx_info(CK_CTX_TEST);
  send_loc_info("abc124.c", 22);
  send_loc_info("abc125.c", 25);
  send_failure_info("Oops");
  tr = receive_test_result(0);
  teardown_messaging();

  ck_assert_msg (tr != NULL,
	       "No test result received");
  ck_assert_msg (tr_ctx(tr) == CK_CTX_TEST,
	       "Bad CTX received");
  ck_assert_msg (strcmp(tr_msg(tr), "Oops") == 0,
	       "Bad failure msg received");
  ck_assert_msg (strcmp(tr_lfile(tr), "abc125.c") == 0,
	       "Bad loc file received");
  ck_assert_msg (tr_lno(tr) == 25,
	       "Bad loc line received");
  if (tr != NULL)
    free(tr);
}
END_TEST

START_TEST(test_send_big)
{
  TestResult *tr;
  int i;
  
  setup_messaging();
  send_ctx_info(CK_CTX_SETUP);
  send_loc_info("abc123.c", 10);
  for (i = 0; i < 10000; i++) {
    send_ctx_info(CK_CTX_TEST);
    send_loc_info("abc124.c", i);
  }

  tr = receive_test_result(0);
  teardown_messaging();

  ck_assert_msg (tr != NULL,
	       "No test result received");
  ck_assert_msg (tr_ctx(tr) == CK_CTX_TEST,
	       "Bad CTX received");
  ck_assert_msg (strcmp(tr_lfile(tr), "abc124.c") == 0,
	       "Bad loc file received");
  ck_assert_msg (tr_lno(tr) == i -1,
	       "Bad loc line received");
  if (tr != NULL)
    tr_free(tr);
}
END_TEST


START_TEST(test_send_test_error)
{
  TestResult *tr;
  setup_messaging();
  send_ctx_info(CK_CTX_SETUP);
  send_loc_info("abc123.c", 10);
  send_ctx_info(CK_CTX_TEST);
  send_loc_info("abc124.c", 22);
  send_loc_info("abc125.c", 25);
  tr = receive_test_result(1);
  teardown_messaging();

  ck_assert_msg (tr != NULL,
	       "No test result received");
  ck_assert_msg (tr_ctx(tr) == CK_CTX_TEST,
	       "Bad CTX received");
  ck_assert_msg (strcmp(tr_lfile(tr), "abc125.c") == 0,
	       "Bad loc file received");
  ck_assert_msg (tr_lno(tr) == 25,
	       "Bad loc line received");
  if (tr != NULL)
    tr_free(tr);
}
END_TEST

START_TEST(test_send_with_passing_teardown)
{
  TestResult *tr;
  setup_messaging();
  send_ctx_info(CK_CTX_SETUP);
  send_loc_info("abc123.c", 10);
  send_ctx_info(CK_CTX_TEST);
  send_loc_info("abc124.c", 22);
  send_loc_info("abc125.c", 25);
  send_ctx_info(CK_CTX_TEARDOWN);
  send_loc_info("abc126.c", 54);
  tr = receive_test_result(0);
  teardown_messaging();

  ck_assert_msg (tr != NULL,
	       "No test result received");
  ck_assert_msg (tr_ctx(tr) == CK_CTX_TEST,
	       "Bad CTX received");
  ck_assert_msg (tr_msg(tr) == NULL,
	       "Bad failure msg received");
  ck_assert_msg (strcmp(tr_lfile(tr), "abc125.c") == 0,
	       "Bad loc file received");
  ck_assert_msg (tr_lno(tr) == 25,
	       "Bad loc line received");
  if (tr != NULL)
    tr_free(tr);
}
END_TEST

START_TEST(test_send_with_error_teardown)
{
  TestResult *tr;
  setup_messaging();
  send_ctx_info(CK_CTX_SETUP);
  send_loc_info("abc123.c", 10);
  send_ctx_info(CK_CTX_TEST);
  send_loc_info("abc124.c", 22);
  send_loc_info("abc125.c", 25);
  send_ctx_info(CK_CTX_TEARDOWN);
  send_loc_info("abc126.c", 54);
  tr = receive_test_result(1);
  teardown_messaging();

  ck_assert_msg (tr != NULL,
	       "No test result received");
  ck_assert_msg (tr_ctx(tr) == CK_CTX_TEARDOWN,
	       "Bad CTX received");
  ck_assert_msg (tr_msg(tr) == NULL,
	       "Bad failure msg received");
  ck_assert_msg (strcmp(tr_lfile(tr), "abc126.c") == 0,
	       "Bad loc file received");
  ck_assert_msg (tr_lno(tr) == 54,
	       "Bad loc line received");
  if (tr != NULL)
    tr_free(tr);
}
END_TEST


Suite *make_msg_suite (void)
{
  Suite *s;
  TCase *tc;
  s = suite_create("Msg");
  tc = tcase_create("Core Tests");
  tcase_add_test(tc, test_send);
  tcase_add_test(tc, test_send_big);
  tcase_add_test(tc, test_send_test_error);
  tcase_add_test(tc, test_send_with_passing_teardown);
  tcase_add_test(tc, test_send_with_error_teardown);
  suite_add_tcase(s, tc);
  return s;
}
