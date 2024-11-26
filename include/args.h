#ifndef ARGS_H
#define ARGS_H

#include "usage.h"
#include <netinet/in.h>

_Noreturn void usage(const char *prog_name, int exit_code, const char *message);
void           parse_args(int argc, char *argv[], char **addr, char **port);
void           handle_args(const char *prog_name, const char *addr, const char *port_str, in_port_t *port);

#endif    // ARGS_H
