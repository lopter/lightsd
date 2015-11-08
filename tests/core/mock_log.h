#pragma once

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
lgtd_err(int eval, const char *fmt, ...)
{
    fprintf(stderr, "ERR: ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(errno));
    exit(eval);
}

void
lgtd_errx(int eval, const char *fmt, ...)
{
    fprintf(stderr, "ERR: ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(eval);
}

void
lgtd_warn(const char *fmt, ...)
{
    if (lgtd_opts.verbosity > LGTD_WARN) {
        return;
    }

    fprintf(stderr, "WARN: ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ": %s\n", strerror(errno));
}

void
lgtd_warnx(const char *fmt, ...)
{
    if (lgtd_opts.verbosity > LGTD_WARN) {
        return;
    }

    fprintf(stderr, "WARN: ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

void
lgtd_info(const char *fmt, ...)
{
    if (lgtd_opts.verbosity > LGTD_INFO) {
        return;
    }

    fprintf(stderr, "INFO: ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

void
lgtd_debug(const char *fmt, ...)
{
    if (lgtd_opts.verbosity > LGTD_DEBUG) {
        return;
    }

    fprintf(stderr, "DEBUG: ");
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}
