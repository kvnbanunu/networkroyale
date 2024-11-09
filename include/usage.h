#ifndef USAGE_H
#define USAGE_H

typedef void (*usage_func)(const char *, int, const char *);

_Noreturn void usage_s(const char *prog_name, int exit_code, const char *message);
_Noreturn void usage_c(const char *prog_name, int exit_code, const char *message);

#endif    // USAGE_H
