#include "../lib/libcompat.h"

#include <sys/types.h>
#include <stdlib.h>
#include <signal.h>
#include <check.h>
#include "check_check.h"

#define _STR(Y) #Y


START_TEST(test_lno)
{
  fail("Failure expected");
  #define LINENO_lno _STR(__LINE__)
}
END_TEST

START_TEST(test_mark_lno)
{
  mark_point();
  #define LINENO_mark_lno _STR(__LINE__)
#ifdef _POSIX_VERSION
  exit(EXIT_FAILURE); /* should fail with mark_point above as line */
#endif /* _POSIX_VERSION */
}

END_TEST
START_TEST(test_pass)
{
  fail_unless(1 == 1, "This test should pass");
  fail_unless(9999, "This test should pass");
}
END_TEST

/* FIXME: this should really be called test_fail_unless */
START_TEST(test_fail)
{
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
  fail_if(1 == 1, "This test should fail");
}
END_TEST

START_TEST(test_fail_null_msg)
{
  fail_unless(2 == 3, NULL);
}
END_TEST

#if defined(__GNUC__)
START_TEST(test_fail_no_msg)
{ /* taking out the NULL provokes an ISO C99 warning in GCC */
  fail_unless(4 == 5, NULL);
}
END_TEST
#endif /* __GNUC__ */
START_TEST(test_fail_if_null_msg)
{
  fail_if(2 != 3, NULL);
}
END_TEST

#if defined(__GNUC__)
START_TEST(test_fail_if_no_msg)
{ /* taking out the NULL provokes an ISO C99 warning in GCC */
  fail_if(4 != 5, NULL);
}
END_TEST
#endif /* __GNUC__ */
START_TEST(test_fail_vararg_msg_1)
{
  int x = 3;
  int y = 4;
  fail_unless(x == y, "%d != %d", x, y);
}
END_TEST

START_TEST(test_fail_vararg_msg_2)
{
  int x = 5;
  int y = 6;
  fail_if(x != y, "%d != %d", x, y);
}
END_TEST

START_TEST(test_fail_vararg_msg_3)
{
  int x = 7;
  int y = 7;
  fail("%d == %d", x, y);
}
END_TEST

#if defined(__GNUC__)
START_TEST(test_fail_empty)
{ /* plain fail() doesn't compile with xlc in C mode because of `, ## __VA_ARGS__' problem */
  /* on the other hand, taking out the NULL provokes an ISO C99 warning in GCC */
  fail(NULL);
}
END_TEST
#endif /* __GNUC__ */

START_TEST(test_ck_abort)
{
  ck_abort();
  #define LINENO_ck_abort _STR(__LINE__)
}
END_TEST

START_TEST(test_ck_abort_msg)
{
  ck_abort_msg("Failure expected");
  #define LINENO_ck_abort_msg _STR(__LINE__)
}
END_TEST

/* FIXME: perhaps passing NULL to ck_abort_msg should be an error. */
START_TEST(test_ck_abort_msg_null)
{
  ck_abort_msg(NULL);
  #define LINENO_ck_abort_msg_null _STR(__LINE__)
}
END_TEST

START_TEST(test_ck_assert)
{
  int x = 3;
  int y = 3;
  ck_assert(1);
  ck_assert(x == y);
  y++;
  ck_assert(x != y);
  ck_assert(x == y);
  #define LINENO_ck_assert _STR(__LINE__)
}
END_TEST

START_TEST(test_ck_assert_null)
{
  ck_assert(0);
  #define LINENO_ck_assert_null _STR(__LINE__)
}
END_TEST

START_TEST(test_ck_assert_int_eq)
{
  int x = 3;
  int y = 3;
  ck_assert_int_eq(x, y);
  y++;
  ck_assert_int_eq(x, y);
  #define LINENO_ck_assert_int_eq _STR(__LINE__)
}
END_TEST

START_TEST(test_ck_assert_int_ne)
{
  int x = 3;
  int y = 2;
  ck_assert_int_ne(x, y);
  y++;
  ck_assert_int_ne(x, y);
  #define LINENO_ck_assert_int_ne _STR(__LINE__)
}
END_TEST

START_TEST(test_ck_assert_str_eq)
{
  const char *s = "test2";
  ck_assert_str_eq("test2", s);
  ck_assert_str_eq("test1", s);
  #define LINENO_ck_assert_str_eq _STR(__LINE__)
}
END_TEST

START_TEST(test_ck_assert_str_ne)
{
  const char *s = "test2";
  const char *t = "test1";
  ck_assert_str_ne(t, s);
  t = "test2";
  ck_assert_str_ne(t, s);
  #define LINENO_ck_assert_str_ne _STR(__LINE__)
}
END_TEST

START_TEST(test_segv)
  #define LINENO_segv _STR(__LINE__)
{
  raise (SIGSEGV);
}
END_TEST


START_TEST(test_fpe)
{
  raise (SIGFPE);
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
  raise(SIGFPE);
  fail("Shouldn't reach here");
}
END_TEST

#if TIMEOUT_TESTS_ENABLED
START_TEST(test_eternal)
  #define LINENO_eternal _STR(__LINE__)
{
  for (;;)
    ;
}
END_TEST

START_TEST(test_sleep2)
{
  sleep(2);
}
END_TEST

START_TEST(test_sleep5)
  #define LINENO_sleep5 _STR(__LINE__)
{
  sleep(5);
}
END_TEST

START_TEST(test_sleep9)
  #define LINENO_sleep9 _STR(__LINE__)
{
  sleep(9);
}
END_TEST

START_TEST(test_sleep14)
  #define LINENO_sleep14 _STR(__LINE__)
{
  sleep(14);
}
END_TEST
#endif

START_TEST(test_early_exit)
{
  exit(EXIT_FAILURE);
}
END_TEST

START_TEST(test_null)
{  
  Suite *s;
  TCase *tc;
  SRunner *sr;
  
  s = suite_create(NULL);
  tc = tcase_create(NULL);
  suite_add_tcase (s, NULL);
  tcase_add_test (tc, NULL);
  sr = srunner_create(NULL);
  srunner_run_all (NULL, -1);
  srunner_free (NULL);
  fail("Completed properly");
}
END_TEST

START_TEST(test_null_2)
{
  SRunner *sr = srunner_create(NULL);
  srunner_run_all (sr, CK_NORMAL);
  srunner_free (sr);
  fail("Completed properly");
}
END_TEST

#ifdef _POSIX_VERSION
START_TEST(test_fork1p_pass)
{
  pid_t pid;

  if((pid = fork()) < 0) {
    fail("Failed to fork new process");
  } else if (pid > 0) {
    fail_unless(1, NULL);
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
    fail("Failed to fork new process");
  } else if (pid > 0) {
    fail("Expected fail");
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
    fail("Failed to fork new process");
  } else if (pid > 0) {
    check_waitpid_and_exit(pid);
  } else {
    fail_unless(1, NULL);
    check_waitpid_and_exit(0);
  }
}
END_TEST

START_TEST(test_fork1c_fail)
{
  pid_t pid;
  
  if((pid = check_fork()) < 0) {
    fail("Failed to fork new process");
  } else if (pid == 0) {
    fail("Expected fail");
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
    fail("Failed to fork new process");
  } else if (pid > 0) {
    if((pid2 = check_fork()) < 0) {
      fail("Failed to fork new process");
    } else if (pid2 == 0) {
      fail_unless(1, NULL);
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
    fail("Failed to fork new process");
  } else if (pid > 0) {
    if((pid2 = check_fork()) < 0) {
      fail("Failed to fork new process");
    } else if (pid2 == 0) {
      fail("Expected fail");
      check_waitpid_and_exit(0);
    }
    check_waitpid_and_exit(pid2);
    fail("Expected fail");
  }
  check_waitpid_and_exit(pid);
}
END_TEST
#endif /* _POSIX_VERSION */

START_TEST(test_srunner)
{
  Suite *s;
  SRunner *sr;

  s = suite_create("Check Servant3");
  fail_unless(s != NULL, NULL);
  sr = srunner_create(NULL);
  fail_unless(sr != NULL, NULL);
  srunner_add_suite(sr, s);
  srunner_free(sr);

  sr = srunner_create(NULL);
  fail_unless(sr != NULL, NULL);
  srunner_add_suite(sr, NULL);
  srunner_free(sr);

  s = suite_create("Check Servant3");
  fail_unless(s != NULL, NULL);
  sr = srunner_create(s);
  fail_unless(sr != NULL, NULL);
  srunner_free(sr);
}
END_TEST

START_TEST(test_2nd_suite)
{
  fail("We failed");
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

void init_master_tests_lineno(void) {
  const char * lineno[] = {
/* Simple Tests */
    LINENO_lno,
    LINENO_mark_lno,
    "-1",
    "-1",
    "-1",
    "-1",
    "-1",
    "-1",
    "-1",
    "-1",
    "-1",
    "-1",
    "-1",
    "-1",
    LINENO_ck_abort,
    LINENO_ck_abort_msg,
    LINENO_ck_abort_msg_null,
    LINENO_ck_assert,
    LINENO_ck_assert_null,
    LINENO_ck_assert_int_eq,
    LINENO_ck_assert_int_ne,
    LINENO_ck_assert_str_eq,
    LINENO_ck_assert_str_ne,

/* Signal Tests */
    "-1",
    "-1",
    LINENO_segv,
    "-1",
    "-1",
    "-1",
    "-1",

#if TIMEOUT_TESTS_ENABLED
/* Timeout Tests */
#if HAVE_WORKING_SETENV
    LINENO_eternal,
    "-1",
    "-1",
    LINENO_sleep9,
#endif
    LINENO_eternal,
    "-1",
    LINENO_sleep5,
    LINENO_sleep9,
    LINENO_eternal,
    "-1",
    "-1",
    LINENO_sleep9,
    LINENO_eternal,
    "-1",
    LINENO_sleep5,
    LINENO_sleep9,
#if HAVE_WORKING_SETENV
    LINENO_eternal,
    "-1",
    "-1",
    LINENO_sleep14,
    LINENO_eternal,
    "-1",
    "-1",
    LINENO_sleep9,
    LINENO_eternal,
    "-1",
    "-1",
    LINENO_sleep14,
#endif
#endif

/* Limit Tests */
    "-1",
    "-1",
    "-1",

/* Msg and fork Tests */
    "-1",
    "-1",
    "-1",
    "-1",
    "-1",
    "-1",

/* Core */
    "-1",
    "-1"
  };
  int s = sizeof lineno /sizeof lineno[0];
  int i;

  for (i = 0; i < s; i++) {
    master_tests_lineno[i] = atoi(lineno[i]) - 1;
  }
}

Suite *make_sub_suite(void)
{
  Suite *s;

  TCase *tc_simple;
  TCase *tc_signal;
#if TIMEOUT_TESTS_ENABLED
#if HAVE_WORKING_SETENV
  TCase *tc_timeout_env;
#endif /* HAVE_WORKING_SETENV */
  TCase *tc_timeout;
  TCase *tc_timeout_usr;
#if HAVE_WORKING_SETENV
  TCase *tc_timeout_env_scale;
  TCase *tc_timeout_scale;
  TCase *tc_timeout_usr_scale;
#endif /* HAVE_WORKING_SETENV */
#endif
  TCase *tc_limit;
  TCase *tc_messaging_and_fork;

  s = suite_create("Check Servant");

  tc_simple = tcase_create("Simple Tests");
  tc_signal = tcase_create("Signal Tests");
#if TIMEOUT_TESTS_ENABLED
#if HAVE_WORKING_SETENV
  setenv("CK_DEFAULT_TIMEOUT", "6", 1);
  tc_timeout_env = tcase_create("Environment Timeout Tests");
  unsetenv("CK_DEFAULT_TIMEOUT");
#endif /* HAVE_WORKING_SETENV */
  tc_timeout = tcase_create("Timeout Tests");
  tc_timeout_usr = tcase_create("User Timeout Tests");
#if HAVE_WORKING_SETENV
  setenv("CK_TIMEOUT_MULTIPLIER", "2", 1);
  tc_timeout_scale = tcase_create("Timeout Scaling Tests");
  tc_timeout_usr_scale = tcase_create("User Timeout Scaling Tests");
  setenv("CK_DEFAULT_TIMEOUT", "6", 1);
  tc_timeout_env_scale = tcase_create("Environment Timeout Scaling Tests");
  unsetenv("CK_DEFAULT_TIMEOUT");
  unsetenv("CK_TIMEOUT_MULTIPLIER");
#endif
#endif
  tc_limit = tcase_create("Limit Tests");
  tc_messaging_and_fork = tcase_create("Msg and fork Tests");

  suite_add_tcase (s, tc_simple);
  suite_add_tcase (s, tc_signal);
#if TIMEOUT_TESTS_ENABLED
#if HAVE_WORKING_SETENV
  suite_add_tcase (s, tc_timeout_env);
#endif /* HAVE_WORKING_SETENV */
  suite_add_tcase (s, tc_timeout);
  suite_add_tcase (s, tc_timeout_usr);
  /* Add a second time to make sure tcase_set_timeout doesn't contaminate it. */
  suite_add_tcase (s, tc_timeout);
#if HAVE_WORKING_SETENV
  suite_add_tcase (s, tc_timeout_env_scale);
  suite_add_tcase (s, tc_timeout_scale);
  suite_add_tcase (s, tc_timeout_usr_scale);
#endif
#endif
  suite_add_tcase (s, tc_limit);
  suite_add_tcase (s, tc_messaging_and_fork);

  tcase_add_test (tc_simple, test_lno);
  tcase_add_test (tc_simple, test_mark_lno);
  tcase_add_test (tc_simple, test_pass);
  tcase_add_test (tc_simple, test_fail);
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
  tcase_add_test (tc_simple, test_ck_assert_int_eq);
  tcase_add_test (tc_simple, test_ck_assert_int_ne);
  tcase_add_test (tc_simple, test_ck_assert_str_eq);
  tcase_add_test (tc_simple, test_ck_assert_str_ne);

  tcase_add_test (tc_signal, test_segv);
  tcase_add_test_raise_signal (tc_signal, test_segv, 11); /* pass  */
  tcase_add_test_raise_signal (tc_signal, test_segv, 8);  /* error */
  tcase_add_test_raise_signal (tc_signal, test_pass, 8);  /* fail  */
  tcase_add_test_raise_signal (tc_signal, test_fail, 8);  /* fail  */
  tcase_add_test (tc_signal, test_fpe);
  tcase_add_test (tc_signal, test_mark_point);

#if TIMEOUT_TESTS_ENABLED
#if HAVE_WORKING_SETENV
  tcase_add_test (tc_timeout_env, test_eternal);
  tcase_add_test (tc_timeout_env, test_sleep2);
  tcase_add_test (tc_timeout_env, test_sleep5);
  tcase_add_test (tc_timeout_env, test_sleep9);
#endif /* HAVE_WORKING_SETENV */

  tcase_add_test (tc_timeout, test_eternal);
  tcase_add_test (tc_timeout, test_sleep2);
  tcase_add_test (tc_timeout, test_sleep5);
  tcase_add_test (tc_timeout, test_sleep9);

  tcase_set_timeout (tc_timeout_usr, 6);
  tcase_add_test (tc_timeout_usr, test_eternal);
  tcase_add_test (tc_timeout_usr, test_sleep2);
  tcase_add_test (tc_timeout_usr, test_sleep5);
  tcase_add_test (tc_timeout_usr, test_sleep9);
#if HAVE_WORKING_SETENV
  tcase_add_test (tc_timeout_env_scale, test_eternal);
  tcase_add_test (tc_timeout_env_scale, test_sleep5);
  tcase_add_test (tc_timeout_env_scale, test_sleep9);
  tcase_add_test (tc_timeout_env_scale, test_sleep14);
  tcase_add_test (tc_timeout_scale, test_eternal);
  tcase_add_test (tc_timeout_scale, test_sleep2);
  tcase_add_test (tc_timeout_scale, test_sleep5);
  tcase_add_test (tc_timeout_scale, test_sleep9);
  setenv("CK_TIMEOUT_MULTIPLIER", "2", 1);
  tcase_set_timeout (tc_timeout_usr_scale, 6);
  unsetenv("CK_TIMEOUT_MULTIPLIER");
  tcase_add_test (tc_timeout_usr_scale, test_eternal);
  tcase_add_test (tc_timeout_usr_scale, test_sleep5);
  tcase_add_test (tc_timeout_usr_scale, test_sleep9);
  tcase_add_test (tc_timeout_usr_scale, test_sleep14);
#endif
#if 0
  tcase_set_timeout (tc_timeout_kill, 2);
  tcase_add_test (tc_timeout_kill, test_sleep);
#endif
#endif

  tcase_add_test (tc_limit, test_early_exit);
  tcase_add_test (tc_limit, test_null);
  tcase_add_test (tc_limit, test_null_2);

#ifdef _POSIX_VERSION
  tcase_add_test (tc_messaging_and_fork, test_fork1p_pass);
  tcase_add_test (tc_messaging_and_fork, test_fork1p_fail);
  tcase_add_test (tc_messaging_and_fork, test_fork1c_pass);
  tcase_add_test (tc_messaging_and_fork, test_fork1c_fail);
  tcase_add_test (tc_messaging_and_fork, test_fork2_pass);
  tcase_add_test (tc_messaging_and_fork, test_fork2_fail);
#endif /* _POSIX_VERSION */

  return s;
}
