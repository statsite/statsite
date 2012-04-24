#include "../lib/libcompat.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "check.h"
#include "check_pack.h"
#include "check_error.h"
#include "check_check.h"

static char errm[512];


START_TEST(test_pack_fmsg)
{
  FailMsg *fmsg;
  char *buf;
  enum ck_msg_type type;

  fmsg = emalloc (sizeof (FailMsg));

  fmsg->msg = (char *) "Hello, world!";
  pack (CK_MSG_FAIL, &buf, (CheckMsg *) fmsg);

  fmsg->msg = (char *) "";
  upack (buf, (CheckMsg *) fmsg, &type);

  fail_unless (type == CK_MSG_FAIL,
	       "Bad type unpacked for FailMsg");

  if (strcmp (fmsg->msg, "Hello, world!") != 0) {
    snprintf (errm, sizeof(errm),
	      "Unpacked string is %s, should be Hello, World!",
	      fmsg->msg);
    fail (errm);
  }

  free (fmsg->msg);
  free (fmsg);
  free (buf);
}
END_TEST

START_TEST(test_pack_loc)
{
  LocMsg *lmsg;
  char *buf;
  enum ck_msg_type type;

  lmsg = emalloc (sizeof (LocMsg));
  lmsg->file = (char *) "abc123.c";
  lmsg->line = 125;

  pack (CK_MSG_LOC, &buf, (CheckMsg *) lmsg);
  lmsg->file = NULL;
  lmsg->line = 0;
  upack (buf, (CheckMsg *) lmsg, &type);

  fail_unless (type == CK_MSG_LOC,
	       "Bad type unpacked for LocMsg");

  if (lmsg->line != 125) {
    snprintf (errm, sizeof (errm),
	     "LocMsg line was %d, should be %d",
	     lmsg->line, 125);
    fail (errm);
  }
  
  if (strcmp (lmsg->file, "abc123.c") != 0) {
    snprintf (errm, sizeof (errm),
              "LocMsg file was %s, should be abc123.c",
              lmsg->file);
    fail (errm);
  }

  free (lmsg->file);
  free (lmsg);
  free (buf);
}
END_TEST

START_TEST(test_pack_ctx)
{
  CtxMsg cmsg;
  char *buf;
  enum ck_msg_type type;
  int npk, nupk;

  cmsg.ctx = CK_CTX_SETUP;
  npk = pack (CK_MSG_CTX, &buf, (CheckMsg *) &cmsg);

  cmsg.ctx = CK_CTX_TEARDOWN;
  nupk = upack (buf, (CheckMsg *) &cmsg, &type);

  fail_unless (type == CK_MSG_CTX,
	       "Bad type unpacked for CtxMsg");

  if (cmsg.ctx != CK_CTX_SETUP) {
    snprintf (errm, sizeof (errm),
	     "CtxMsg ctx got %d, expected %d",
	     cmsg.ctx, CK_CTX_SETUP);
    fail (errm);
  }

  free (buf);
}
END_TEST


START_TEST(test_pack_len)
{
  CtxMsg cmsg;
  char *buf;
  int n = 0;
  enum ck_msg_type type;

  cmsg.ctx = CK_CTX_TEST;
  n = pack (CK_MSG_CTX, &buf, (CheckMsg *) &cmsg);
  fail_unless (n > 0, "Return val from pack not set correctly");

  /* Value below may change with different implementations of pack */
  fail_unless (n == 8, "Return val from pack not correct");
  n = 0;
  n = upack (buf, (CheckMsg *) &cmsg, &type);
  if (n != 8) {
    snprintf (errm, sizeof (errm), "%d bytes read from upack, should be 8", n);
    fail (errm);
  }
  
  free (buf);
}
END_TEST

START_TEST(test_pack_ctx_limit)
{
  CtxMsg cmsg;
  CtxMsg *cmsgp = NULL;
  char *buf;

  cmsg.ctx = -1;
  pack (CK_MSG_CTX, &buf, (CheckMsg *) &cmsg);
  pack (CK_MSG_CTX, &buf, (CheckMsg *) cmsgp);
}
END_TEST

START_TEST(test_pack_fail_limit)
{
  FailMsg fmsg;
  FailMsg *fmsgp = NULL;
  char *buf;
  enum ck_msg_type type;

  fmsg.msg = (char *) "";
  pack (CK_MSG_FAIL, &buf, (CheckMsg *) &fmsg);
  fmsg.msg = (char *) "abc";
  upack (buf, (CheckMsg *) &fmsg, &type);
  free (buf);
  fail_unless (strcmp (fmsg.msg, "") == 0, 
               "Empty string not handled properly");

  free (fmsg.msg);
  fmsg.msg = NULL;

  pack (CK_MSG_FAIL, &buf, (CheckMsg *) &fmsg);
  pack (CK_MSG_FAIL, &buf, (CheckMsg *) fmsgp);
}
END_TEST

START_TEST(test_pack_loc_limit)
{
  LocMsg lmsg;
  LocMsg *lmsgp = NULL;
  char *buf;
  enum ck_msg_type type;

  lmsg.file = (char *) "";
  lmsg.line = 0;
  pack (CK_MSG_LOC, &buf, (CheckMsg *) &lmsg);
  lmsg.file = (char *) "abc";
  upack (buf, (CheckMsg *) &lmsg, &type);
  fail_unless (strcmp (lmsg.file, "") == 0,
	       "Empty string not handled properly");
  free (lmsg.file);
  lmsg.file = NULL;

  pack (CK_MSG_LOC, &buf, (CheckMsg *) &lmsg);
  pack (CK_MSG_LOC, &buf, (CheckMsg *) lmsgp);
}
END_TEST

/* the ppack probably means 'pipe' pack */
#ifdef _POSIX_VERSION
START_TEST(test_ppack)
{
  int filedes[2];
  CtxMsg cmsg;
  LocMsg lmsg;
  FailMsg fmsg;
  RcvMsg *rmsg;

  cmsg.ctx = CK_CTX_TEST;
  lmsg.file = (char *) "abc123.c";
  lmsg.line = 10;
  fmsg.msg = (char *) "oops";
  pipe (filedes);
  ppack (filedes[1], CK_MSG_CTX, (CheckMsg *) &cmsg);
  ppack (filedes[1], CK_MSG_LOC, (CheckMsg *) &lmsg);
  ppack (filedes[1], CK_MSG_FAIL, (CheckMsg *) &fmsg);
  close (filedes[1]);
  rmsg = punpack (filedes[0]);

  fail_unless (rmsg != NULL,
	       "Return value from ppack should always be malloc'ed");
  fail_unless (rmsg->lastctx == CK_CTX_TEST,
	       "CTX not set correctly in ppack");
  fail_unless (rmsg->fixture_line == -1,
	       "Default fixture loc not correct");
  fail_unless (rmsg->fixture_file == NULL,
	       "Default fixture loc not correct");
  fail_unless (rmsg->test_line == 10,
	       "Test line not received correctly");
  fail_unless (strcmp(rmsg->test_file,"abc123.c") == 0,
	       "Test file not received correctly");
  fail_unless (strcmp(rmsg->msg, "oops") == 0,
	       "Failure message not received correctly");

  free(rmsg);
}
END_TEST

START_TEST(test_ppack_noctx)
{
  int filedes[2];
  LocMsg lmsg;
  FailMsg fmsg;
  RcvMsg *rmsg;

  lmsg.file = (char *) "abc123.c";
  lmsg.line = 10;
  fmsg.msg = (char *) "oops";
  pipe (filedes);
  ppack (filedes[1], CK_MSG_LOC, (CheckMsg *) &lmsg);
  ppack (filedes[1], CK_MSG_FAIL, (CheckMsg *) &fmsg);
  close (filedes[1]);
  rmsg = punpack (filedes[0]);

  fail_unless (rmsg == NULL,
	       "Result should be NULL with no CTX");

  if (rmsg != NULL)
    free (rmsg);
}
END_TEST

START_TEST(test_ppack_onlyctx)
{
  int filedes[2];
  CtxMsg cmsg;
  RcvMsg *rmsg;

  cmsg.ctx = CK_CTX_SETUP;
  pipe (filedes);
  ppack (filedes[1], CK_MSG_CTX, (CheckMsg *) &cmsg);
  close (filedes[1]);
  rmsg = punpack (filedes[0]);

  fail_unless (rmsg->msg == NULL,
	       "Result message should be NULL with only CTX");
  fail_unless (rmsg->fixture_line == -1,
	       "Result loc line should be -1 with only CTX");
  fail_unless (rmsg->test_line == -1,
	       "Result loc line should be -1 with only CTX");

  if (rmsg != NULL)
    free (rmsg);
}
END_TEST

START_TEST(test_ppack_multictx)
{
  int filedes[2];
  CtxMsg cmsg;
  LocMsg lmsg;
  RcvMsg *rmsg;

  cmsg.ctx = CK_CTX_SETUP;
  lmsg.line = 5;
  lmsg.file = (char *) "abc123.c";
  pipe (filedes);
  ppack (filedes[1], CK_MSG_CTX, (CheckMsg *) &cmsg);
  ppack (filedes[1], CK_MSG_LOC, (CheckMsg *) &lmsg);
  cmsg.ctx = CK_CTX_TEST;
  ppack (filedes[1], CK_MSG_CTX, (CheckMsg *) &cmsg);
  ppack (filedes[1], CK_MSG_LOC, (CheckMsg *) &lmsg);
  cmsg.ctx = CK_CTX_TEARDOWN;
  ppack (filedes[1], CK_MSG_CTX, (CheckMsg *) &cmsg);
  close (filedes[1]);
  rmsg = punpack (filedes[0]);

  fail_unless (rmsg->test_line == 5,
	       "Test loc not being preserved on CTX change");

  fail_unless (rmsg->fixture_line == -1,
	       "Fixture not reset on CTX change");
  if (rmsg != NULL)
    free (rmsg);
}
END_TEST

START_TEST(test_ppack_nofail)
{
  int filedes[2];
  CtxMsg cmsg;
  LocMsg lmsg;
  RcvMsg *rmsg;

  lmsg.file = (char *) "abc123.c";
  lmsg.line = 10;
  cmsg.ctx = CK_CTX_SETUP;
  pipe (filedes);
  ppack (filedes[1], CK_MSG_CTX, (CheckMsg *) &cmsg);
  ppack (filedes[1], CK_MSG_LOC, (CheckMsg *) &lmsg);
  close (filedes[1]);
  rmsg = punpack (filedes[0]);

  fail_unless (rmsg->msg == NULL,
	       "Failure result should be NULL with no failure message");
  if (rmsg != NULL)
    free (rmsg);
}
END_TEST

#define BIG_MSG_LEN 1037

START_TEST(test_ppack_big)
{
  int filedes[2];
  CtxMsg cmsg;
  LocMsg lmsg;
  FailMsg fmsg;
  RcvMsg *rmsg;

  cmsg.ctx = CK_CTX_TEST;
  lmsg.file = emalloc (BIG_MSG_LEN);
  memset (lmsg.file,'a',BIG_MSG_LEN - 1);
  lmsg.file[BIG_MSG_LEN - 1] = '\0';
  lmsg.line = 10;
  fmsg.msg = emalloc (BIG_MSG_LEN);
  memset (fmsg.msg, 'a', BIG_MSG_LEN - 1);
  fmsg.msg[BIG_MSG_LEN - 1] = '\0';
  pipe (filedes);
  ppack (filedes[1], CK_MSG_CTX, (CheckMsg *) &cmsg);
  ppack (filedes[1], CK_MSG_LOC, (CheckMsg *) &lmsg);
  ppack (filedes[1], CK_MSG_FAIL, (CheckMsg *) &fmsg);
  close (filedes[1]);
  rmsg = punpack (filedes[0]);

  fail_unless (rmsg != NULL,
	       "Return value from ppack should always be malloc'ed");
  fail_unless (rmsg->lastctx == CK_CTX_TEST,
	       "CTX not set correctly in ppack");
  fail_unless (rmsg->test_line == 10,
	       "Test line not received correctly");
  fail_unless (strcmp (rmsg->test_file, lmsg.file) == 0,
	       "Test file not received correctly");
  fail_unless (strcmp (rmsg->msg, fmsg.msg) == 0,
	       "Failure message not received correctly");
  
  free (rmsg);
  free (lmsg.file);
  free (fmsg.msg);
}
END_TEST
#endif /* _POSIX_VERSION */

Suite *make_pack_suite(void)
{

  Suite *s;
  TCase *tc_core;
  TCase *tc_limit;

  s = suite_create ("Pack");
  tc_core = tcase_create ("Core");
  tc_limit = tcase_create ("Limit");

  suite_add_tcase (s, tc_core);
  tcase_add_test (tc_core, test_pack_fmsg);
  tcase_add_test (tc_core, test_pack_loc);
  tcase_add_test (tc_core, test_pack_ctx);
  tcase_add_test (tc_core, test_pack_len);
#ifdef _POSIX_VERSION
  tcase_add_test (tc_core, test_ppack);
  tcase_add_test (tc_core, test_ppack_noctx);
  tcase_add_test (tc_core, test_ppack_onlyctx);
  tcase_add_test (tc_core, test_ppack_multictx);
  tcase_add_test (tc_core, test_ppack_nofail);
#endif /* _POSIX_VERSION */
  suite_add_tcase (s, tc_limit);
  tcase_add_test (tc_limit, test_pack_ctx_limit);
  tcase_add_test (tc_limit, test_pack_fail_limit);
  tcase_add_test (tc_limit, test_pack_loc_limit);
#ifdef _POSIX_VERSION
  tcase_add_test (tc_limit, test_ppack_big);
#endif /* _POSIX_VERSION */

  return s;
}
