#ifndef ARGS_H
#define ARGS_H

#include <netinet/in.h>

void parse_args_s(int argc, char *argv[], char **addr, char **port);
void handle_args_s(const char *prog_name, const char *addr, const char *port_str, in_port_t *port);

void parse_args_c(int argc, char *argv[]);
void handle_args_c(const char *prog_name);

#endif // ARGS_H
#define ARGS_H
