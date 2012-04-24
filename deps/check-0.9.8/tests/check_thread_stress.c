#include "../lib/libcompat.h"

#include <stdlib.h>
#include <stdio.h>
#include <check.h>

Suite *s;
TCase *tc;
SRunner *sr;

#if defined (HAVE_PTHREAD) || defined (_POSIX_VERSION)
static void *
sendinfo (void *userdata CK_ATTRIBUTE_UNUSED)
{
  unsigned int i;
  for (i = 0; i < 999; i++)
    {
      fail_unless (1, "Shouldn't see this message");
    }
  return NULL;
}
#endif /* HAVE_PTHREAD || _POSIX_VERSION */

#ifdef HAVE_PTHREAD
START_TEST (test_stress_threads)
{
  pthread_t a, b;
  pthread_create (&a, NULL, sendinfo, (void *) 0xa);
  pthread_create (&b, NULL, sendinfo, (void *) 0xb);

  pthread_join (a, NULL);
  pthread_join (b, NULL);
}
END_TEST
#endif /* HAVE_PTHREAD */

#ifdef _POSIX_VERSION
START_TEST (test_stress_forks)
{
  pid_t cpid = fork ();
  if (cpid == 0)
    {
      /* child */
      sendinfo ((void *) 0x1);
      exit (EXIT_SUCCESS);
    }
  else
    {
      /* parent */
      sendinfo ((void *) 0x2);
    }
}
END_TEST
#endif /* _POSIX_VERSION */

int
main (void)
{
  int nf;
  s = suite_create ("ForkThreadStress");
  tc = tcase_create ("ForkThreadStress");
  sr = srunner_create (s);
  suite_add_tcase (s, tc);

#ifdef HAVE_PTHREAD
  tcase_add_loop_test (tc, test_stress_threads, 0, 100);
#endif /* HAVE_PTHREAD */

#ifdef _POSIX_VERSION
  tcase_add_loop_test (tc, test_stress_forks, 0, 100);
#endif /* _POSIX_VERSION */

  srunner_run_all (sr, CK_VERBOSE);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  /* hack to give us XFAIL on non-posix platforms */
#ifndef _POSIX_VERSION
  nf++;
#endif /* !_POSIX_VERSION */

  return nf ? EXIT_FAILURE : EXIT_SUCCESS;
}
