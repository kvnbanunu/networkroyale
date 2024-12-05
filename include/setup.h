#ifndef SETUP_H
#define SETUP_H

#include <netinet/in.h>

#define AP_LEN 6

_Noreturn void usage(const char *prog_name, int exit_code, const char *message);
void parse_args(int argc, char **argv, char **address, char **port_str, in_port_t *port);
void find_address(in_addr_t *address, char *address_str);
int setup_server(struct sockaddr_in *addr);
void find_port(struct sockaddr_in *addr, const char host_address[INET_ADDRSTRLEN]);
void setup_client_known(struct sockaddr_in *addr, const char *addr_str, in_port_t port);
void send_client_info(int fd, const struct sockaddr_in *local, struct sockaddr_in *remote, socklen_t addr_len);
void setup_client_unknown(struct sockaddr_in *addr, const uint8_t buf[AP_LEN]);

#endif    // SETUP_H
