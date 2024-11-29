#ifndef SETUP_H
#define SETUP_H

#include <netinet/in.h>

void      findaddress(in_addr_t *address, char *address_str);
in_port_t setup_and_bind(int *sockfd, struct sockaddr_in *addr, in_addr_t address, socklen_t addr_len, int type, int flag);
void      setup_and_connect(int *sockfd, struct sockaddr_in *addr, const char *address, in_port_t port, socklen_t addr_len);
void      socket_close(int sockfd);

#endif    // SETUP_H
