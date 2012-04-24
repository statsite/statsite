#ifndef LIBCOMPAT_H
#define LIBCOMPAT_H

#if HAVE_CONFIG_H
#include <config.h>
#endif

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define GCC_VERSION_AT_LEAST(major, minor) \
((__GNUC__ > (major)) || \
 (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
#define GCC_VERSION_AT_LEAST(major, minor) 0
#endif

#if GCC_VERSION_AT_LEAST(2,95)
#define CK_ATTRIBUTE_UNUSED __attribute__ ((unused))
#else
#define CK_ATTRIBUTE_UNUSED              
#endif /* GCC 2.95 */

/* defines size_t */
#include <sys/types.h>

/* provides assert */
#include <assert.h>

/* defines FILE */
#include <stdio.h>

/* provides localtime and struct tm */
#include <sys/time.h>
#include <time.h>

/* declares fork(), _POSIX_VERSION.  according to Autoconf.info,
   unistd.h defines _POSIX_VERSION if the system is POSIX-compliant,
   so we will use this as a test for all things uniquely provided by
   POSIX like sigaction() and fork() */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

/* declares pthread_create and friends */
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

/* replacement functions for broken originals */
#if !HAVE_MALLOC
void *rpl_malloc (size_t n);
#endif /* !HAVE_MALLOC */

#if !HAVE_REALLOC
void *rpl_realloc (void *p, size_t n);
#endif /* !HAVE_REALLOC */

/* functions that may be undeclared */
#if !HAVE_DECL_FILENO
int fileno (FILE *stream);
#endif /* !HAVE_DECL_FILENO */

#if !HAVE_DECL_LOCALTIME_R
struct tm *localtime_r (const time_t *clock, struct tm *result);
#endif /* !HAVE_DECL_LOCALTIME_R */

#if !HAVE_DECL_PIPE
int pipe (int *fildes);
#endif /* !HAVE_DECL_PIPE */

#if !HAVE_DECL_PUTENV
int putenv (const char *string);
#endif /* !HAVE_DECL_PUTENV */

#if !HAVE_DECL_SETENV
int setenv (const char *name, const char *value, int overwrite);
#endif /* !HAVE_DECL_SETENV */

/* our setenv implementation is currently broken */
#if !HAVE_SETENV
#define HAVE_WORKING_SETENV 0
#else
#define HAVE_WORKING_SETENV 1
#endif

#if !HAVE_DECL_SLEEP
unsigned int sleep (unsigned int seconds);
#endif /* !HAVE_DECL_SLEEP */

#if !HAVE_DECL_STRDUP
char *strdup (const char *str);
#endif /* !HAVE_DECL_STRDUP */

#if !HAVE_DECL_STRSIGNAL
const char *strsignal (int sig);
#endif /* !HAVE_DECL_STRSIGNAL */

#if !HAVE_DECL_UNSETENV
void unsetenv (const char *name);
#endif /* !HAVE_DECL_UNSETENV */

/* silence warnings about an empty library */
void ck_do_nothing (void);

#endif /* !LIBCOMPAT_H */
