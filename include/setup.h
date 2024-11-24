#ifndef SETUP_H
#define SETUP_H

#include <netinet/in.h>

void findaddress(in_addr_t *address, char *address_str);
void setup_tcp_server(int *sockfd, struct sockaddr_in *addr, in_addr_t address, const socklen_t *addr_len);

#endif    // SETUP_H
