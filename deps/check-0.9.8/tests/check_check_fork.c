#include "../lib/libcompat.h"

#include <sys/types.h>
#include <stdlib.h>
#include <check.h>
#include "check_check.h"


static int counter;
static pid_t mypid;

static void fork_sub_setup (void)
{
  counter = 0;
  mypid = getpid();
}

START_TEST(test_inc)
{
  counter++;
}
END_TEST

START_TEST(test_nofork_sideeffects)
{
  fail_unless(counter == 1,
	      "Side effects not seen across tests");
}
END_TEST

START_TEST(test_nofork_pid)
{
  fail_unless(mypid == getpid(),
	      "Unit test is in a different adresss space from setup code");
}
END_TEST

static Suite *make_fork_sub_suite (void)
{

  Suite *s;
  TCase *tc;

  s = suite_create("Fork Sub");
  tc = tcase_create("Core");

  suite_add_tcase (s, tc);
  tcase_add_unchecked_fixture(tc, fork_sub_setup,NULL);
  tcase_add_test(tc,test_inc);
  tcase_add_test(tc,test_nofork_sideeffects);
  tcase_add_test(tc,test_nofork_pid);

  return s;
}

static SRunner *fork_sr;
static SRunner *fork_dummy_sr;

void fork_setup (void)
{
  fork_sr = srunner_create(make_fork_sub_suite());
  fork_dummy_sr = srunner_create (make_fork_sub_suite());
  srunner_set_fork_status(fork_sr,CK_NOFORK);
  srunner_run_all(fork_sr,CK_VERBOSE);
}

void fork_teardown (void)
{
  srunner_free(fork_sr);
}

START_TEST(test_default_fork)
{
  fail_unless(srunner_fork_status(fork_dummy_sr) == CK_FORK,
	      "Default fork status not set correctly");
}
END_TEST

START_TEST(test_set_fork)
{
  srunner_set_fork_status(fork_dummy_sr, CK_NOFORK);
  fail_unless(srunner_fork_status(fork_dummy_sr) == CK_NOFORK,
	      "Fork status not changed correctly");
}
END_TEST

START_TEST(test_env)
{
  putenv((char *) "CK_FORK=no");
  fail_unless(srunner_fork_status(fork_dummy_sr) == CK_NOFORK,
	      "Fork status does not obey environment variable");
}
END_TEST

START_TEST(test_env_and_set)
{
  putenv((char *) "CK_FORK=no");
  srunner_set_fork_status(fork_dummy_sr, CK_FORK);  
  fail_unless(srunner_fork_status(fork_dummy_sr) == CK_FORK,
	      "Explicit setting of fork status should override env");
}
END_TEST


START_TEST(test_nofork)
{
  fail_unless(srunner_ntests_failed(fork_sr) == 0,
	      "Errors on nofork test");
}
END_TEST

Suite *make_fork_suite(void)
{
  Suite *s;
  TCase *tc;

  s = suite_create("Fork");
  tc = tcase_create("Core");

  suite_add_tcase(s, tc);
  tcase_add_test(tc,test_default_fork);
  tcase_add_test(tc,test_set_fork);
  tcase_add_test(tc,test_env);
  tcase_add_test(tc,test_env_and_set);
  tcase_add_test(tc,test_nofork);
  
  return s;
}
