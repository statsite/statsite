#include "../lib/libcompat.h"

#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <check.h>
#include "check_check.h"

START_TEST(test_lno)
{
  record_failure_line_num(__LINE__);
  ck_abort_msg("Failure expected");
}
END_TEST

#if defined(HAVE_FORK) && HAVE_FORK==1
START_TEST(test_mark_lno)
{
  record_failure_line_num(__LINE__);
  mark_point();
  exit(EXIT_FAILURE); /* should fail with mark_point above as line */
}
END_TEST
#endif /* HAVE_FORK */

START_TEST(test_pass)
{
  ck_assert_msg(1 == 1, "This test should pass");
  ck_assert_msg(9999, "This test should pass");
}
END_TEST

START_TEST(test_fail_unless)
{
  record_failure_line_num(__LINE__);
  fail_unless(1 == 2, "This test should fail");
}
END_TEST

START_TEST(test_fail_if_pass)
{
  fail_if(1 == 2, "This test should pass");
  fail_if(0, "This test should pass");
}
END_TEST

START_TEST(test_fail_if_fail)
{
  record_failure_line_num(__LINE__);
  fail_if(1 == 1, "This test should fail");
}
END_TEST

START_TEST(test_fail_null_msg)
{
  record_failure_line_num(__LINE__);
  fail_unless(2 == 3, NULL);
}
END_TEST

#if defined(__GNUC__)
START_TEST(test_fail_no_msg)
{ /* taking out the NULL provokes an ISO C99 warning in GCC */
  record_failure_line_num(__LINE__);
  fail_unless(4 == 5, NULL);
}
END_TEST
#endif /* __GNUC__ */
START_TEST(test_fail_if_null_msg)
{
  record_failure_line_num(__LINE__);
  fail_if(2 != 3, NULL);
}
END_TEST

#if defined(__GNUC__)
START_TEST(test_fail_if_no_msg)
{ /* taking out the NULL provokes an ISO C99 warning in GCC */
  record_failure_line_num(__LINE__);
  fail_if(4 != 5, NULL);
}
END_TEST
#endif /* __GNUC__ */
START_TEST(test_fail_vararg_msg_1)
{
  int x = 3;
  int y = 4;
  record_failure_line_num(__LINE__);
  fail_unless(x == y, "%d != %d", x, y);
}
END_TEST

START_TEST(test_fail_vararg_msg_2)
{
  int x = 5;
  int y = 6;
  record_failure_line_num(__LINE__);
  fail_if(x != y, "%d != %d", x, y);
}
END_TEST

START_TEST(test_fail_vararg_msg_3)
{
  int x = 7;
  int y = 7;
  record_failure_line_num(__LINE__);
  fail("%d == %d", x, y);
}
END_TEST

#if defined(__GNUC__)
START_TEST(test_fail_empty)
{ /* plain fail() doesn't compile with xlc in C mode because of `, ## __VA_ARGS__' problem */
  /* on the other hand, taking out the NULL provokes an ISO C99 warning in GCC */
  record_failure_line_num(__LINE__);
  fail(NULL);
}
END_TEST
#endif /* __GNUC__ */

START_TEST(test_ck_abort)
{
  record_failure_line_num(__LINE__);
  ck_abort();
}
END_TEST

START_TEST(test_ck_abort_msg)
{
  record_failure_line_num(__LINE__);
  ck_abort_msg("Failure expected");
}
END_TEST

/* FIXME: perhaps passing NULL to ck_abort_msg should be an error. */
START_TEST(test_ck_abort_msg_null)
{
  record_failure_line_num(__LINE__);
  ck_abort_msg(NULL);
}
END_TEST

/* These ck_assert tests are all designed to fail on the last
   assertion. */

START_TEST(test_ck_assert)
{
  int x = 3;
  int y = 3;
  ck_assert(1);
  ck_assert(x == y);
  y++;
  ck_assert(x != y);
  record_failure_line_num(__LINE__);
  ck_assert(x == y);
}
END_TEST

START_TEST(test_ck_assert_null)
{
  record_failure_line_num(__LINE__);
  ck_assert(0);
}
END_TEST

START_TEST(test_ck_assert_with_mod)
{
  int f = 1;
  record_failure_line_num(__LINE__);
  ck_assert(1%f == 1);
}
END_TEST

START_TEST(test_ck_assert_int_eq)
{
  int x = 3;
  int y = 3;
  ck_assert_int_eq(x, y);
  y++;
  record_failure_line_num(__LINE__);
  ck_assert_int_eq(x, y);
}
END_TEST

START_TEST(test_ck_assert_int_eq_with_mod)
{
  int d = 2;
  int f = 1;
  record_failure_line_num(__LINE__);
  ck_assert_int_eq(3%d, 2%f);
}
END_TEST

START_TEST(test_ck_assert_int_ne)
{
  int x = 3;
  int y = 2;
  ck_assert_int_ne(x, y);
  y++;
  record_failure_line_num(__LINE__);
  ck_assert_int_ne(x, y);
}
END_TEST

START_TEST(test_ck_assert_int_ne_with_mod)
{
  int d = 2;
  int f = 2;
  record_failure_line_num(__LINE__);
  ck_assert_int_ne(3%d, 3%f);
}
END_TEST

START_TEST(test_ck_assert_int_lt)
{
  int x = 2;
  int y = 3;
  ck_assert_int_lt(x, y);
  record_failure_line_num(__LINE__);
  ck_assert_int_lt(x, x);
}
END_TEST

START_TEST(test_ck_assert_int_lt_with_mod)
{
  int d = 2;
  int f = 1;
  record_failure_line_num(__LINE__);
  ck_assert_int_lt(3%d, 3%f);
}
END_TEST

START_TEST(test_ck_assert_int_le)
{
  int x = 2;
  int y = 3;
  ck_assert_int_le(x, y);
  ck_assert_int_le(x, x);
  record_failure_line_num(__LINE__);
  ck_assert_int_le(y, x);
}
END_TEST

START_TEST(test_ck_assert_int_le_with_mod)
{
  int d = 2;
  int f = 1;
  record_failure_line_num(__LINE__);
  ck_assert_int_le(3%d, 2%f);
}
END_TEST

START_TEST(test_ck_assert_int_gt)
{
  int x = 2;
  int y = 3;
  ck_assert_int_gt(y, x);
  record_failure_line_num(__LINE__);
  ck_assert_int_gt(y, y);
}
END_TEST

START_TEST(test_ck_assert_int_gt_with_mod)
{
  int d = 1;
  int f = 2;
  record_failure_line_num(__LINE__);
  ck_assert_int_gt(3%d, 3%f);
}
END_TEST

START_TEST(test_ck_assert_int_ge)
{
  int x = 2;
  int y = 3;
  ck_assert_int_ge(y, x);
  ck_assert_int_ge(y, x);
  record_failure_line_num(__LINE__);
  ck_assert_int_ge(x, y);
}
END_TEST

START_TEST(test_ck_assert_int_ge_with_mod)
{
  int d = 1;
  int f = 3;
  record_failure_line_num(__LINE__);
  ck_assert_int_ge(3%d, 4%f);
}
END_TEST

START_TEST(test_ck_assert_int_expr)
{
  int x = 1;
  int y = 0;
  ck_assert_int_eq(x, ++y);
  ck_assert_int_eq(x, y);
} END_TEST

START_TEST(test_ck_assert_uint_eq)
{
  unsigned int x = 3;
  unsigned int y = 3;
  ck_assert_uint_eq(x, y);
  y++;
  record_failure_line_num(__LINE__);
  ck_assert_uint_eq(x, y);
}
END_TEST

START_TEST(test_ck_assert_uint_eq_with_mod)
{
  int d = 2;
  int f = 1;
  record_failure_line_num(__LINE__);
  ck_assert_uint_eq(3%d, 1%f);
}
END_TEST

START_TEST(test_ck_assert_uint_ne)
{
  unsigned int x = 3;
  unsigned int y = 2;
  ck_assert_uint_ne(x, y);
  y++;
  record_failure_line_num(__LINE__);
  ck_assert_uint_ne(x, y);
}
END_TEST

START_TEST(test_ck_assert_uint_ne_with_mod)
{
  int d = 1;
  int f = 1;
  record_failure_line_num(__LINE__);
  ck_assert_uint_ne(1%d, 1%f);
}
END_TEST

START_TEST(test_ck_assert_uint_lt)
{
  unsigned int x = 2;
  unsigned int y = 3;
  ck_assert_uint_lt(x, y);
  record_failure_line_num(__LINE__);
  ck_assert_uint_lt(x, x);
}
END_TEST

START_TEST(test_ck_assert_uint_lt_with_mod)
{
  int d = 2;
  int f = 1;
  record_failure_line_num(__LINE__);
  ck_assert_uint_lt(3%d, 1%f);
}
END_TEST

START_TEST(test_ck_assert_uint_le)
{
  unsigned int x = 2;
  unsigned int y = 3;
  ck_assert_uint_le(x, y);
  ck_assert_uint_le(x, x);
  record_failure_line_num(__LINE__);
  ck_assert_uint_le(y, x);
}
END_TEST

START_TEST(test_ck_assert_uint_le_with_mod)
{
  int d = 2;
  int f = 1;
  record_failure_line_num(__LINE__);
  ck_assert_uint_le(3%d, 1%f);
}
END_TEST

START_TEST(test_ck_assert_uint_gt)
{
  unsigned int x = 2;
  unsigned int y = 3;
  ck_assert_uint_gt(y, x);
  record_failure_line_num(__LINE__);
  ck_assert_uint_gt(y, y);
}
END_TEST

START_TEST(test_ck_assert_uint_gt_with_mod)
{
  int d = 1;
  int f = 2;
  record_failure_line_num(__LINE__);
  ck_assert_uint_gt(1%d, 3%f);
}
END_TEST

START_TEST(test_ck_assert_uint_ge)
{
  unsigned int x = 2;
  unsigned int y = 3;
  ck_assert_uint_ge(y, x);
  ck_assert_uint_ge(y, x);
  record_failure_line_num(__LINE__);
  ck_assert_uint_ge(x, y);
}
END_TEST

START_TEST(test_ck_assert_uint_ge_with_mod)
{
  int d = 1;
  int f = 2;
  record_failure_line_num(__LINE__);
  ck_assert_uint_ge(1%d, 3%f);
}
END_TEST

START_TEST(test_ck_assert_uint_expr)
{
  unsigned int x = 1;
  unsigned int y = 0;
  ck_assert_uint_eq(x, ++y);
  ck_assert_uint_eq(x, y);
} END_TEST

START_TEST(test_ck_assert_str_eq)
{
  const char *s = "test2";
  ck_assert_str_eq("test2", s);
  record_failure_line_num(__LINE__);
  ck_assert_str_eq("test1", s);
}
END_TEST

START_TEST(test_ck_assert_str_ne)
{
  const char *s = "test2";
  const char *t = "test1";
  ck_assert_str_ne(t, s);
  t = "test2";
  record_failure_line_num(__LINE__);
  ck_assert_str_ne(t, s);
}
END_TEST

START_TEST(test_ck_assert_str_lt)
{
  const char *s = "test1";
  const char *t = "test2";
  ck_assert_str_lt(s, t);
  record_failure_line_num(__LINE__);
  ck_assert_str_lt(s, s);
}
END_TEST

START_TEST(test_ck_assert_str_le)
{
  const char *s = "test1";
  const char *t = "test2";
  ck_assert_str_le(s, t);
  ck_assert_str_le(s, s);
  record_failure_line_num(__LINE__);
  ck_assert_str_le(t, s);
}
END_TEST

START_TEST(test_ck_assert_str_gt)
{
  const char *s = "test1";
  const char *t = "test2";
  ck_assert_str_gt(t, s);
  record_failure_line_num(__LINE__);
  ck_assert_str_gt(t, t);
}
END_TEST

START_TEST(test_ck_assert_str_ge)
{
  const char *s = "test1";
  const char *t = "test2";
  ck_assert_str_ge(t, s);
  ck_assert_str_ge(t, t);
  record_failure_line_num(__LINE__);
  ck_assert_str_ge(s, t);
}
END_TEST

START_TEST(test_ck_assert_str_expr)
{
  const char *s = "test1";
  const char *t[] = { "test1", "test2" };
  int i = -1;
  ck_assert_str_eq(s, t[++i]);
  ck_assert_str_eq(s, t[i]);
}
END_TEST

START_TEST(test_ck_assert_ptr_eq)
{
  int * x = (int*)0x1;
  int * y = (int*)0x2;
  ck_assert_ptr_eq(NULL, NULL);
  ck_assert_ptr_eq(x,    x);
  record_failure_line_num(__LINE__);
  ck_assert_ptr_eq(x,    y);
}
END_TEST

START_TEST(test_ck_assert_ptr_ne)
{
  int * x = (int*)0x1;
  int * y = (int*)0x2;
  int * z = x;
  ck_assert_ptr_ne(x,    y);
  ck_assert_ptr_ne(x,    NULL);
  ck_assert_ptr_ne(NULL, y);
  record_failure_line_num(__LINE__);
  ck_assert_ptr_ne(x,    z);
}
END_TEST

#if defined(HAVE_FORK) && HAVE_FORK == 1
START_TEST(test_segv_pass)
{
  /*
   * This test is to be used when it would otherwise not cause a
   * failure. e.g., shen SIGSEGV is expected.
   */
  raise (SIGSEGV);
}
END_TEST

START_TEST(test_segv)
{
  record_failure_line_num(__LINE__-3); /* -3 as the failure is reported at START_TEST() */
  raise (SIGSEGV);
}
END_TEST

START_TEST(test_fpe)
{
  record_failure_line_num(__LINE__-3); /* -3 as the failure is reported at START_TEST() */
  raise (SIGFPE);
}
END_TEST

/*
 * This test is to be used when the test is expected to throw signal 8,
 * but does not, resulting in a failure.
 */
START_TEST(test_non_signal_8)
{
  record_failure_line_num(__LINE__-3); /* -3 as the failure is reported at START_TEST() */
  exit(0);
}
END_TEST

/* TODO:
   unit test running the same suite in succession */

START_TEST(test_mark_point)
{
  int i;
  i = 0;
  i++;
  mark_point();
  record_failure_line_num(__LINE__-2); /* -2 as the failure is listed as from mark_point() */
  raise(SIGFPE);
  ck_abort_msg("Shouldn't reach here");
}
END_TEST
#endif

#if TIMEOUT_TESTS_ENABLED && defined(HAVE_FORK) && HAVE_FORK == 1
START_TEST(test_eternal_fail)
{
  record_failure_line_num(__LINE__-3); /* -3 as the failure is reported at START_TEST() */
  for (;;)
    sleep(1);
}
END_TEST

/* 
 * Only include sub-second timing tests on systems
 * that support librt.
 */
#ifdef HAVE_LIBRT
START_TEST(test_sleep0_025_pass)
{
  usleep(25*1000);
}
END_TEST

START_TEST(test_sleep1_pass)
{
  sleep(1);
}
END_TEST

START_TEST(test_sleep1_fail)
{
  record_failure_line_num(__LINE__-3); /* -3 as the failure is reported at START_TEST() */
  sleep(1);
}
END_TEST
#endif /* HAVE_LIBRT */

START_TEST(test_sleep2_pass)
{
  sleep(2);
}
END_TEST

START_TEST(test_sleep2_fail)
{
  record_failure_line_num(__LINE__-3); /* -3 as the failure is reported at START_TEST() */
  sleep(2);
}
END_TEST

START_TEST(test_sleep5_pass)
{
  sleep(5);
}
END_TEST

START_TEST(test_sleep5_fail)
{
  record_failure_line_num(__LINE__-3); /* -3 as the failure is reported at START_TEST() */
  sleep(5);
}
END_TEST

START_TEST(test_sleep9_pass)
{
  sleep(9);
}
END_TEST

START_TEST(test_sleep9_fail)
{
  record_failure_line_num(__LINE__-3); /* -3 as the failure is reported at START_TEST() */
  sleep(9);
}
END_TEST

START_TEST(test_sleep14_fail)
{
  record_failure_line_num(__LINE__-3); /* -3 as the failure is reported at START_TEST() */
  sleep(14);
}
END_TEST
#endif /* TIMEOUT_TESTS_ENABLED && defined(HAVE_FORK) */

#if defined(HAVE_FORK) && HAVE_FORK==1
START_TEST(test_early_exit)
{
  record_failure_line_num(__LINE__-3); /* -3 as the failure is reported at START_TEST() */
  exit(EXIT_FAILURE);
}
END_TEST
#endif /* HAVE_FORK */

/*
 * The following test will leak memory because it is calling
 * APIs inproperly. The leaked memory cannot be free'd, as
 * the methods to do so are static. (No user of Check should
 * call them directly).
 */
#if MEMORY_LEAKING_TESTS_ENABLED
START_TEST(test_null)
{  
  Suite *s;
  TCase *tc;
  
  s = suite_create(NULL);
  tc = tcase_create(NULL);
  suite_add_tcase (s, NULL);
  tcase_add_test (tc, NULL);
  srunner_free(srunner_create(NULL));
  srunner_run_all (NULL, (enum print_output)-1);
  srunner_free (NULL);
  record_failure_line_num(__LINE__);
  ck_abort_msg("Completed properly");
}
END_TEST
#endif /* MEMORY_LEAKING_TESTS_ENABLED */

START_TEST(test_null_2)
{
  SRunner *sr = srunner_create(NULL);
  srunner_run_all (sr, CK_NORMAL);
  srunner_free (sr);
  ck_assert_int_eq(suite_tcase(NULL, NULL), 0);
  record_failure_line_num(__LINE__);
  ck_abort_msg("Completed properly");
}
END_TEST

#if defined(HAVE_FORK) && HAVE_FORK==1
START_TEST(test_fork1p_pass)
{
  pid_t pid;

  if((pid = fork()) < 0) {
    ck_abort_msg("Failed to fork new process");
  } else if (pid > 0) {
    ck_assert_msg(1, NULL);
    kill(pid, SIGKILL);
  } else {
    for (;;) {
      sleep(1);
    }
  }
}
END_TEST

START_TEST(test_fork1p_fail)
{
  pid_t pid;
  
  if((pid = fork()) < 0) {
    ck_abort_msg("Failed to fork new process");
  } else if (pid > 0) {
    record_failure_line_num(__LINE__);
    ck_abort_msg("Expected fail");
    kill(pid, SIGKILL);
  } else {
    for (;;) {
      sleep(1);
    }
  }
}
END_TEST

START_TEST(test_fork1c_pass)
{
  pid_t pid;
  
  if((pid = check_fork()) < 0) {
    ck_abort_msg("Failed to fork new process");
  } else if (pid > 0) {
    check_waitpid_and_exit(pid);
  } else {
    ck_assert_msg(1, NULL);
    check_waitpid_and_exit(0);
  }
}
END_TEST

START_TEST(test_fork1c_fail)
{
  pid_t pid;
  
  if((pid = check_fork()) < 0) {
    ck_abort_msg("Failed to fork new process");
  } else if (pid == 0) {
    record_failure_line_num(__LINE__);
    ck_abort_msg("Expected fail");
    check_waitpid_and_exit(0);
  }
  check_waitpid_and_exit(pid);
}
END_TEST

START_TEST(test_fork2_pass)
{
  pid_t pid;
  pid_t pid2;
  
  if((pid = check_fork()) < 0) {
    ck_abort_msg("Failed to fork new process");
  } else if (pid > 0) {
    if((pid2 = check_fork()) < 0) {
      ck_abort_msg("Failed to fork new process");
    } else if (pid2 == 0) {
      ck_assert_msg(1, NULL);
      check_waitpid_and_exit(0);
    }
    check_waitpid_and_exit(pid2);
  }
  check_waitpid_and_exit(pid);
}
END_TEST

START_TEST(test_fork2_fail)
{
  pid_t pid;
  pid_t pid2;
  
  if((pid = check_fork()) < 0) {
    ck_abort_msg("Failed to fork new process");
  } else if (pid > 0) {
    if((pid2 = check_fork()) < 0) {
      ck_abort_msg("Failed to fork new process");
    } else if (pid2 == 0) {
      record_failure_line_num(__LINE__);
      ck_abort_msg("Expected fail");
      check_waitpid_and_exit(0);
    }
    check_waitpid_and_exit(pid2);
    ck_abort_msg("Expected fail");
  }
  check_waitpid_and_exit(pid);
}
END_TEST
#endif /* HAVE_FORK */

#if defined(HAVE_FORK) && HAVE_FORK == 1
#if MEMORY_LEAKING_TESTS_ENABLED
START_TEST(test_invalid_set_fork_status)
{
   Suite *s1;
   TCase *tc1;
   SRunner *sr;
   record_failure_line_num(__LINE__-6); /* -6 as the failure is reported at START_TEST() */
   s1 = suite_create ("suite1");
   tc1 = tcase_create ("tcase1");
   tcase_add_test (tc1, test_pass);
   sr = srunner_create(s1);
   srunner_set_fork_status (sr, (enum fork_status)-1);
   srunner_run_all(sr, CK_SILENT);
}
END_TEST
#endif /* MEMORY_LEAKING_TESTS_ENABLED */
#endif /* HAVE_FORK */

START_TEST(test_srunner)
{
  Suite *s;
  SRunner *sr;

  s = suite_create("Check Servant3");
  ck_assert_msg(s != NULL, NULL);
  sr = srunner_create(NULL);
  ck_assert_msg(sr != NULL, NULL);
  srunner_add_suite(sr, s);
  srunner_free(sr);

  sr = srunner_create(NULL);
  ck_assert_msg(sr != NULL, NULL);
  srunner_add_suite(sr, NULL);
  srunner_free(sr);

  s = suite_create("Check Servant3");
  ck_assert_msg(s != NULL, NULL);
  sr = srunner_create(s);
  ck_assert_msg(sr != NULL, NULL);
  srunner_free(sr);
}
END_TEST

START_TEST(test_2nd_suite)
{
  record_failure_line_num(__LINE__);
  ck_abort_msg("We failed");
}
END_TEST

Suite *make_sub2_suite(void)
{
  Suite *s = suite_create("Check Servant2");
  TCase *tc = tcase_create("Core");
  suite_add_tcase(s, tc);
  tcase_add_test(tc, test_srunner);
  tcase_add_test(tc, test_2nd_suite);

  return s;
}

#if defined(HAVE_FORK) && HAVE_FORK == 1
void exit_handler(void);
void exit_handler ()
{
  /* This exit handler should never be executed */
  while(1)
  {
    sleep(1);
  }
}

START_TEST(test_ignore_exit_handlers)
{
  int result = atexit(exit_handler);
  if(result != 0)
  {
    ck_abort_msg("Failed to set an exit handler, test cannot proceed");
  }
  record_failure_line_num(__LINE__);
  ck_abort();
}
END_TEST
#endif /* HAVE_FORK */

Suite *make_sub_suite(void)
{
  Suite *s;

  TCase *tc_simple;
#if defined(HAVE_FORK) && HAVE_FORK==1
  TCase *tc_signal;
#endif
#if TIMEOUT_TESTS_ENABLED && defined(HAVE_FORK) && HAVE_FORK == 1
#if HAVE_DECL_SETENV
  TCase *tc_timeout_env_int;
  TCase *tc_timeout_env_double;
#endif /* HAVE_DECL_SETENV */
  TCase *tc_timeout_default;
  TCase *tc_timeout_usr_int;
  TCase *tc_timeout_usr_double;
#if HAVE_DECL_SETENV
  TCase *tc_timeout_env_scale_int;
  TCase *tc_timeout_scale_int;
  TCase *tc_timeout_usr_scale_int;
  TCase *tc_timeout_env_scale_double;
  TCase *tc_timeout_scale_double;
  TCase *tc_timeout_usr_scale_double;
#endif /* HAVE_DECL_SETENV */
#endif /* TIMEOUT_TESTS_ENABLED && defined(HAVE_FORK) */
  TCase *tc_limit;
#if defined(HAVE_FORK) && HAVE_FORK==1
  TCase *tc_messaging_and_fork;
  TCase *tc_errors;
  TCase *tc_exit_handlers;
#endif

  s = suite_create("Check Servant");

  tc_simple = tcase_create("Simple Tests");
#if defined(HAVE_FORK) && HAVE_FORK==1
  tc_signal = tcase_create("Signal Tests");
#endif /* HAVE_FORK */
#if TIMEOUT_TESTS_ENABLED && defined(HAVE_FORK) && HAVE_FORK == 1
#if HAVE_DECL_SETENV
  setenv("CK_DEFAULT_TIMEOUT", "6", 1);
  tc_timeout_env_int = tcase_create("Environment Integer Timeout Tests");
  unsetenv("CK_DEFAULT_TIMEOUT");
  setenv("CK_DEFAULT_TIMEOUT", "0.5", 1);
  tc_timeout_env_double = tcase_create("Environment Double Timeout Tests");
  unsetenv("CK_DEFAULT_TIMEOUT");
#endif /* HAVE_DECL_SETENV */
  tc_timeout_default = tcase_create("Default Timeout Tests");
  tc_timeout_usr_int = tcase_create("User Integer Timeout Tests");
  tc_timeout_usr_double = tcase_create("User Double Timeout Tests");
#if HAVE_DECL_SETENV
  setenv("CK_TIMEOUT_MULTIPLIER", "2", 1);
  tc_timeout_scale_int = tcase_create("Timeout Integer Scaling Tests");
  tc_timeout_usr_scale_int = tcase_create("User Integer Timeout Scaling Tests");
  setenv("CK_DEFAULT_TIMEOUT", "6", 1);
  tc_timeout_env_scale_int = tcase_create("Environment Integer Timeout Scaling Tests");
  unsetenv("CK_DEFAULT_TIMEOUT");
  unsetenv("CK_TIMEOUT_MULTIPLIER");
  
  setenv("CK_TIMEOUT_MULTIPLIER", "0.4", 1);
  tc_timeout_scale_double = tcase_create("Timeout Double Scaling Tests");
  tc_timeout_usr_scale_double = tcase_create("User Double Timeout Scaling Tests");
  setenv("CK_DEFAULT_TIMEOUT", "0.9", 1);
  tc_timeout_env_scale_double = tcase_create("Environment Double Timeout Scaling Tests");
  unsetenv("CK_DEFAULT_TIMEOUT");
  unsetenv("CK_TIMEOUT_MULTIPLIER");
#endif /* HAVE_DECL_SETENV */
#endif /* TIMEOUT_TESTS_ENABLED && defined(HAVE_FORK) */
  tc_limit = tcase_create("Limit Tests");
#if defined(HAVE_FORK) && HAVE_FORK==1
  tc_messaging_and_fork = tcase_create("Msg and fork Tests");
  tc_errors = tcase_create("Check Errors Tests");
  tc_exit_handlers = tcase_create("Check Ignore Exit Handlers");
#endif /* HAVE_FORK */

  suite_add_tcase (s, tc_simple);
#if defined(HAVE_FORK) && HAVE_FORK==1
  suite_add_tcase (s, tc_signal);
#endif /* HAVE_FORK */
#if TIMEOUT_TESTS_ENABLED && defined(HAVE_FORK) && HAVE_FORK == 1
#if HAVE_DECL_SETENV
  suite_add_tcase (s, tc_timeout_env_int);
  suite_add_tcase (s, tc_timeout_env_double);
#endif /* HAVE_DECL_SETENV */
  suite_add_tcase (s, tc_timeout_default);
  suite_add_tcase (s, tc_timeout_usr_int);
  suite_add_tcase (s, tc_timeout_usr_double);

  /* Add a second time to make sure tcase_set_timeout doesn't contaminate it. */
  suite_add_tcase (s, tc_timeout_default);
#if HAVE_DECL_SETENV
  suite_add_tcase (s, tc_timeout_env_scale_int);
  suite_add_tcase (s, tc_timeout_env_scale_double);
  suite_add_tcase (s, tc_timeout_scale_int);
  suite_add_tcase (s, tc_timeout_scale_double);
  suite_add_tcase (s, tc_timeout_usr_scale_int);
  suite_add_tcase (s, tc_timeout_usr_scale_double);
#endif /* HAVE_DECL_SETENV */
#endif /* TIMEOUT_TESTS_ENABLED && defined(HAVE_FORK) */
  suite_add_tcase (s, tc_limit);
#if defined(HAVE_FORK) && HAVE_FORK == 1
  suite_add_tcase (s, tc_messaging_and_fork);
  suite_add_tcase (s, tc_errors);
  suite_add_tcase (s, tc_exit_handlers);
#endif

  tcase_add_test (tc_simple, test_lno);
#if defined(HAVE_FORK) && HAVE_FORK==1
  tcase_add_test (tc_simple, test_mark_lno);
#endif
  tcase_add_test (tc_simple, test_pass);
  tcase_add_test (tc_simple, test_fail_unless);
  tcase_add_test (tc_simple, test_fail_if_pass);
  tcase_add_test (tc_simple, test_fail_if_fail);
  tcase_add_test (tc_simple, test_fail_null_msg);
#if defined(__GNUC__)
  tcase_add_test (tc_simple, test_fail_no_msg);
#endif /* __GNUC__ */
  tcase_add_test (tc_simple, test_fail_if_null_msg);
#if defined(__GNUC__)
  tcase_add_test (tc_simple, test_fail_if_no_msg);
#endif /* __GNUC__ */
  tcase_add_test (tc_simple, test_fail_vararg_msg_1);
  tcase_add_test (tc_simple, test_fail_vararg_msg_2);
  tcase_add_test (tc_simple, test_fail_vararg_msg_3);
#if defined(__GNUC__)
  tcase_add_test (tc_simple, test_fail_empty);
#endif /* __GNUC__ */

  tcase_add_test (tc_simple, test_ck_abort);
  tcase_add_test (tc_simple, test_ck_abort_msg);
  tcase_add_test (tc_simple, test_ck_abort_msg_null);
  tcase_add_test (tc_simple, test_ck_assert);
  tcase_add_test (tc_simple, test_ck_assert_null);
  tcase_add_test (tc_simple, test_ck_assert_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_int_eq);
  tcase_add_test (tc_simple, test_ck_assert_int_eq_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_int_ne);
  tcase_add_test (tc_simple, test_ck_assert_int_ne_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_int_lt);
  tcase_add_test (tc_simple, test_ck_assert_int_lt_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_int_le);
  tcase_add_test (tc_simple, test_ck_assert_int_le_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_int_gt);
  tcase_add_test (tc_simple, test_ck_assert_int_gt_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_int_ge);
  tcase_add_test (tc_simple, test_ck_assert_int_ge_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_int_expr);
  tcase_add_test (tc_simple, test_ck_assert_uint_eq);
  tcase_add_test (tc_simple, test_ck_assert_uint_eq_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_uint_ne);
  tcase_add_test (tc_simple, test_ck_assert_uint_ne_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_uint_lt);
  tcase_add_test (tc_simple, test_ck_assert_uint_lt_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_uint_le);
  tcase_add_test (tc_simple, test_ck_assert_uint_le_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_uint_gt);
  tcase_add_test (tc_simple, test_ck_assert_uint_gt_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_uint_ge);
  tcase_add_test (tc_simple, test_ck_assert_uint_ge_with_mod);
  tcase_add_test (tc_simple, test_ck_assert_uint_expr);
  tcase_add_test (tc_simple, test_ck_assert_str_eq);
  tcase_add_test (tc_simple, test_ck_assert_str_ne);
  tcase_add_test (tc_simple, test_ck_assert_str_lt);
  tcase_add_test (tc_simple, test_ck_assert_str_le);
  tcase_add_test (tc_simple, test_ck_assert_str_gt);
  tcase_add_test (tc_simple, test_ck_assert_str_ge);
  tcase_add_test (tc_simple, test_ck_assert_str_expr);
  tcase_add_test (tc_simple, test_ck_assert_ptr_eq);
  tcase_add_test (tc_simple, test_ck_assert_ptr_ne);

#if defined(HAVE_FORK) && HAVE_FORK==1
  tcase_add_test (tc_signal, test_segv);
  tcase_add_test_raise_signal (tc_signal, test_segv_pass, 11); /* pass  */
  tcase_add_test_raise_signal (tc_signal, test_segv, 8);  /* error */
  tcase_add_test_raise_signal (tc_signal, test_non_signal_8, 8);  /* fail  */
  tcase_add_test_raise_signal (tc_signal, test_fail_unless, 8);  /* fail  */
  tcase_add_test (tc_signal, test_fpe);
  tcase_add_test (tc_signal, test_mark_point);
#endif /* HAVE_FORK */

#if TIMEOUT_TESTS_ENABLED && defined(HAVE_FORK) && HAVE_FORK == 1
#if HAVE_DECL_SETENV
  /* tc_timeout_env_int tests have a timeout of 6 seconds */
  tcase_add_test (tc_timeout_env_int, test_eternal_fail);
  tcase_add_test (tc_timeout_env_int, test_sleep2_pass);
  tcase_add_test (tc_timeout_env_int, test_sleep5_pass);
  tcase_add_test (tc_timeout_env_int, test_sleep9_fail);
  
  /* tc_timeout_env_double tests have a timeout of 0.5 seconds */
  tcase_add_test (tc_timeout_env_double, test_eternal_fail);
#ifdef HAVE_LIBRT
  tcase_add_test (tc_timeout_env_double, test_sleep0_025_pass);
  tcase_add_test (tc_timeout_env_double, test_sleep1_fail);
#endif /* HAVE_LIBRT */
  tcase_add_test (tc_timeout_env_double, test_sleep2_fail);
  tcase_add_test (tc_timeout_env_double, test_sleep5_fail);
  tcase_add_test (tc_timeout_env_double, test_sleep9_fail);
#endif /* HAVE_DECL_SETENV */

  /* tc_timeout_default tests have a timeout of 4 seconds */
  tcase_add_test (tc_timeout_default, test_eternal_fail);
#ifdef HAVE_LIBRT
  tcase_add_test (tc_timeout_default, test_sleep0_025_pass);
  tcase_add_test (tc_timeout_default, test_sleep1_pass);
#endif /* HAVE_LIBRT */
  tcase_add_test (tc_timeout_default, test_sleep2_pass);
  tcase_add_test (tc_timeout_default, test_sleep5_fail);
  tcase_add_test (tc_timeout_default, test_sleep9_fail);

  tcase_set_timeout (tc_timeout_usr_int, 6);
  tcase_add_test (tc_timeout_usr_int, test_eternal_fail);
  tcase_add_test (tc_timeout_usr_int, test_sleep2_pass);
  tcase_add_test (tc_timeout_usr_int, test_sleep5_pass);
  tcase_add_test (tc_timeout_usr_int, test_sleep9_fail);

  tcase_set_timeout (tc_timeout_usr_double, 0.5);
  tcase_add_test (tc_timeout_usr_double, test_eternal_fail);
#ifdef HAVE_LIBRT
  tcase_add_test (tc_timeout_usr_double, test_sleep0_025_pass);
  tcase_add_test (tc_timeout_usr_double, test_sleep1_fail);
#endif /* HAVE_LIBRT */
  tcase_add_test (tc_timeout_usr_double, test_sleep2_fail);
  tcase_add_test (tc_timeout_usr_double, test_sleep5_fail);
  tcase_add_test (tc_timeout_usr_double, test_sleep9_fail);
  
#if HAVE_DECL_SETENV
  /* tc_timeout_env_scale_int tests have a timeout of 6 (time) * 2 (multiplier) = 12 seconds */
  tcase_add_test (tc_timeout_env_scale_int, test_eternal_fail);
#ifdef HAVE_LIBRT
  tcase_add_test (tc_timeout_env_scale_int, test_sleep0_025_pass);
  tcase_add_test (tc_timeout_env_scale_int, test_sleep1_pass);
#endif /* HAVE_LIBRT */
  tcase_add_test (tc_timeout_env_scale_int, test_sleep2_pass);
  tcase_add_test (tc_timeout_env_scale_int, test_sleep5_pass);
  tcase_add_test (tc_timeout_env_scale_int, test_sleep9_pass);
  tcase_add_test (tc_timeout_env_scale_int, test_sleep14_fail);

  /* tc_timeout_env_scale_double tests have a timeout of 0.9 (time) * 0.4 (multiplier) = 0.36 seconds */
  tcase_add_test (tc_timeout_env_scale_double, test_eternal_fail);
#ifdef HAVE_LIBRT
  tcase_add_test (tc_timeout_env_scale_double, test_sleep0_025_pass);
  tcase_add_test (tc_timeout_env_scale_double, test_sleep1_fail);
#endif /* HAVE_LIBRT */
  tcase_add_test (tc_timeout_env_scale_double, test_sleep2_fail);
  tcase_add_test (tc_timeout_env_scale_double, test_sleep5_fail);
  tcase_add_test (tc_timeout_env_scale_double, test_sleep9_fail);
  tcase_add_test (tc_timeout_env_scale_double, test_sleep14_fail);

  /* tc_timeout_scale_int tests have a timeout of 2 * 4 (default) = 8 seconds */
  tcase_add_test (tc_timeout_scale_int, test_eternal_fail);
#ifdef HAVE_LIBRT
  tcase_add_test (tc_timeout_scale_int, test_sleep0_025_pass);
  tcase_add_test (tc_timeout_scale_int, test_sleep1_pass);
  tcase_add_test (tc_timeout_scale_int, test_sleep2_pass);
#endif /* HAVE_LIBRT */
  tcase_add_test (tc_timeout_scale_int, test_sleep5_pass);
  tcase_add_test (tc_timeout_scale_int, test_sleep9_fail);

  /* tc_timeout_scale_double tests have a timeout of 4 (default) * 0.3 (multiplier) = 1.6 second */
  tcase_add_test (tc_timeout_scale_double, test_eternal_fail);
#ifdef HAVE_LIBRT
  tcase_add_test (tc_timeout_scale_double, test_sleep0_025_pass);
  tcase_add_test (tc_timeout_scale_double, test_sleep1_pass);
#endif /* HAVE_LIBRT */
  tcase_add_test (tc_timeout_scale_double, test_sleep2_fail);
  tcase_add_test (tc_timeout_scale_double, test_sleep5_fail);
  tcase_add_test (tc_timeout_scale_double, test_sleep9_fail);
  
  setenv("CK_TIMEOUT_MULTIPLIER", "2", 1);
  tcase_set_timeout (tc_timeout_usr_scale_int, 6);
  unsetenv("CK_TIMEOUT_MULTIPLIER");
  tcase_add_test (tc_timeout_usr_scale_int, test_eternal_fail);
#ifdef HAVE_LIBRT
  tcase_add_test (tc_timeout_usr_scale_int, test_sleep0_025_pass);
  tcase_add_test (tc_timeout_usr_scale_int, test_sleep1_pass);
#endif /* HAVE_LIBRT */
  tcase_add_test (tc_timeout_usr_scale_int, test_sleep2_pass);
  tcase_add_test (tc_timeout_usr_scale_int, test_sleep5_pass);
  tcase_add_test (tc_timeout_usr_scale_int, test_sleep9_pass);
  tcase_add_test (tc_timeout_usr_scale_int, test_sleep14_fail);
  
  setenv("CK_TIMEOUT_MULTIPLIER", "0.4", 1);
  tcase_set_timeout (tc_timeout_usr_scale_double, 0.9);
  unsetenv("CK_TIMEOUT_MULTIPLIER");
  tcase_add_test (tc_timeout_usr_scale_double, test_eternal_fail);
#ifdef HAVE_LIBRT
  tcase_add_test (tc_timeout_usr_scale_double, test_sleep0_025_pass);
  tcase_add_test (tc_timeout_usr_scale_double, test_sleep1_fail);
#endif /* HAVE_LIBRT */
  tcase_add_test (tc_timeout_usr_scale_double, test_sleep2_fail);
  tcase_add_test (tc_timeout_usr_scale_double, test_sleep5_fail);
  tcase_add_test (tc_timeout_usr_scale_double, test_sleep9_fail);
  tcase_add_test (tc_timeout_usr_scale_double, test_sleep14_fail);
#endif /* HAVE_DECL_SETENV */
#endif /* TIMEOUT_TESTS_ENABLED && defined(HAVE_FORK) */

#if defined(HAVE_FORK) && HAVE_FORK==1
  tcase_add_test (tc_limit, test_early_exit);
#endif /* HAVE_FORK */
#if MEMORY_LEAKING_TESTS_ENABLED
  tcase_add_test (tc_limit, test_null);
#endif /* MEMORY_LEAKING_TESTS_ENABLED */
  tcase_add_test (tc_limit, test_null_2);

#if defined(HAVE_FORK) && HAVE_FORK==1
  tcase_add_test (tc_messaging_and_fork, test_fork1p_pass);
  tcase_add_test (tc_messaging_and_fork, test_fork1p_fail);
  tcase_add_test (tc_messaging_and_fork, test_fork1c_pass);
  tcase_add_test (tc_messaging_and_fork, test_fork1c_fail);
  tcase_add_test (tc_messaging_and_fork, test_fork2_pass);
  tcase_add_test (tc_messaging_and_fork, test_fork2_fail);

#if MEMORY_LEAKING_TESTS_ENABLED
  tcase_add_test_raise_signal (tc_errors, test_invalid_set_fork_status, 2);
#endif

  tcase_add_test (tc_exit_handlers, test_ignore_exit_handlers);
#endif /* HAVE_FORK */

  return s;
}
