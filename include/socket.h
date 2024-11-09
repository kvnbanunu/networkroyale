#ifndef SOCKET_H
#define SOCKET_H

#include "usage.h"
#include <netinet/in.h>

in_port_t parse_in_port_t(const char *prog_name, const char *port_str, usage_func usage);
void      convert_address(const char *address, struct sockaddr_storage *addr);
int       socket_create(int domain, int type, int protocol);
void      socket_bind(int sockfd, struct sockaddr_storage *addr, in_port_t port);
void      socket_close(int sockfd);

#endif    // SOCKET_H
