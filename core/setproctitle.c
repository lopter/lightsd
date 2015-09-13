#if !LGTD_HAVE_SETPROCTITLE
/*
 * Copyright © 2010 William Ahern
 * Copyright © 2012-2013 Guillem Jover <guillem@hadrons.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the
 * following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <errno.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <unistd.h>
#include <string.h>

extern char **environ;

static struct {
	/* Original value. */
	const char *arg0;

	/* Title space available. */
	char *base, *end;

	 /* Pointer to original nul character within base. */
	char *nul;

	bool warned;
	bool reset;
	int error;
} SPT;


static inline size_t
spt_min(size_t a, size_t b)
{
	return a < b ? a : b;
}

/*
 * For discussion on the portability of the various methods, see
 * http://lists.freebsd.org/pipermail/freebsd-stable/2008-June/043136.html
 */
static int
spt_clearenv(void)
{
#ifdef HAVE_CLEARENV
	return clearenv();
#else
	char **tmp;

	tmp = malloc(sizeof(*tmp));
	if (tmp == NULL)
		return errno;

	tmp[0] = NULL;
	environ = tmp;

	return 0;
#endif
}

static int
spt_copyenv(int envc, char *envp[])
{
	char **envcopy;
	char *eq;
	int envsize;
	int i, error;

	if (environ != envp)
		return 0;

	/* Make a copy of the old environ array of pointers, in case
	 * clearenv() or setenv() is implemented to free the internal
	 * environ array, because we will need to access the old environ
	 * contents to make the new copy. */
	envsize = (envc + 1) * sizeof(char *);
	envcopy = malloc(envsize);
	if (envcopy == NULL)
		return errno;
	memcpy(envcopy, envp, envsize);

	error = spt_clearenv();
	if (error) {
		environ = envp;
		free(envcopy);
		return error;
	}

	for (i = 0; envcopy[i]; i++) {
		eq = strchr(envcopy[i], '=');
		if (eq == NULL)
			continue;

		*eq = '\0';
		if (setenv(envcopy[i], eq + 1, 1) < 0)
			error = errno;
		*eq = '=';

		if (error) {
#ifdef HAVE_CLEARENV
			/* Because the old environ might not be available
			 * anymore we will make do with the shallow copy. */
			environ = envcopy;
#else
			environ = envp;
			free(envcopy);
#endif
			return error;
		}
	}

	/* Dispose of the shallow copy, now that we've finished transfering
	 * the old environment. */
	free(envcopy);

	return 0;
}

static int
spt_copyargs(int argc, char *argv[])
{
	char *tmp;
	int i;

	for (i = 1; i < argc || (i >= argc && argv[i]); i++) {
		if (argv[i] == NULL)
			continue;

		tmp = strdup(argv[i]);
		if (tmp == NULL)
			return errno;

		argv[i] = tmp;
	}

	return 0;
}

/* 
  Rejected in glibc (http://sourceware.org/ml/libc-alpha/2006-03/msg00125.html)
*/

#ifdef HAVE___PROGNAME
extern const char *__progname;
#else
static const char *__progname = NULL;
#endif

const char *
getprogname(void)
{
#if defined(HAVE_PROGRAM_INVOCATION_SHORT_NAME)
	if (__progname == NULL)
		__progname = program_invocation_short_name;
#elif defined(HAVE_GETEXECNAME)
	/* getexecname(3) returns an absolute pathname, normalize it. */
	if (__progname == NULL)
		setprogname(getexecname());
#endif

	return __progname;
}

void
setprogname(const char *progname)
{
	const char *last_slash;

	last_slash = strrchr(progname, '/');
	if (last_slash == NULL)
		__progname = progname;
	else
		__progname = last_slash + 1;
}

void
setproctitle_init(int argc, char *argv[], char *envp[])
{
	char *base, *end, *nul, *tmp;
	int i, envc, error;

	/* Try to make sure we got called with main() arguments. */
	if (argc < 0)
		return;

	base = argv[0];
	if (base == NULL)
		return;

	nul = &base[strlen(base)];
	end = nul + 1;

	for (i = 0; i < argc || (i >= argc && argv[i]); i++) {
		if (argv[i] == NULL || argv[i] < end)
			continue;

		end = argv[i] + strlen(argv[i]) + 1;
	}

	for (i = 0; envp[i]; i++) {
		if (envp[i] < end)
			continue;

		end = envp[i] + strlen(envp[i]) + 1;
	}
	envc = i;

	SPT.arg0 = strdup(argv[0]);
	if (SPT.arg0 == NULL) {
		SPT.error = errno;
		return;
	}

	tmp = strdup(getprogname());
	if (tmp == NULL) {
		SPT.error = errno;
		return;
	}
	setprogname(tmp);

	error = spt_copyenv(envc, envp);
	if (error) {
		SPT.error = error;
		return;
	}

	error = spt_copyargs(argc, argv);
	if (error) {
		SPT.error = error;
		return;
	}

	SPT.nul  = nul;
	SPT.base = base;
	SPT.end  = end;
}

#ifndef SPT_MAXTITLE
#define SPT_MAXTITLE 255
#endif

void
setproctitle(const char *fmt, ...)
{
	/* Use buffer in case argv[0] is passed. */
	char buf[SPT_MAXTITLE + 1];
	va_list ap;
	char *nul;
	int len;

	if (SPT.base == NULL) {
		if (!SPT.warned) {
			warnx("setproctitle not initialized, please either call "
			      "setproctitle_init() or link against libbsd-ctor.");
			SPT.warned = true;
		}
		return;
	}

	if (fmt) {
		if (fmt[0] == '-') {
			/* Skip program name prefix. */
			fmt++;
			len = 0;
		} else {
			/* Print program name heading for grep. */
			snprintf(buf, sizeof(buf), "%s: ", getprogname());
			len = strlen(buf);
		}

		va_start(ap, fmt);
		len += vsnprintf(buf + len, sizeof(buf) - len, fmt, ap);
		va_end(ap);
	} else {
		len = snprintf(buf, sizeof(buf), "%s", SPT.arg0);
	}

	if (len <= 0) {
		SPT.error = errno;
		return;
	}

	if (!SPT.reset) {
		memset(SPT.base, 0, SPT.end - SPT.base);
		SPT.reset = true;
	} else {
		memset(SPT.base, 0, spt_min(sizeof(buf), SPT.end - SPT.base));
	}

	len = spt_min(len, spt_min(sizeof(buf), SPT.end - SPT.base) - 1);
	memcpy(SPT.base, buf, len);
	nul = &SPT.base[len];

	if (nul < SPT.nul) {
		*SPT.nul = '.';
	} else if (nul == SPT.nul && &nul[1] < SPT.end) {
		*SPT.nul = ' ';
		*++nul = '\0';
	}
}
#else
typedef int hello_compiler;
#endif
