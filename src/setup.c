#include "../include/setup.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PREFIX "192.168"

void findaddress(in_addr_t *address, char *address_str)
{
    struct ifaddrs       *ifaddr;
    const struct ifaddrs *ifa;

    if(getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if(ifa->ifa_addr == NULL)
        {
            continue;
        }

        if(ifa->ifa_addr->sa_family == AF_INET)
        {
            if(getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), address_str, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST) != 0)
            {
                perror("getnameinfo");
                continue;
            }
            if(strncmp(address_str, PREFIX, strlen(PREFIX)) == 0)
            {
                inet_pton(AF_INET, address_str, address);
                break;
            }
        }
    }
    if(ifa == NULL)
    {
        freeifaddrs(ifaddr);
        perror("no address");
        exit(EXIT_FAILURE);
    }
    freeifaddrs(ifaddr);
}

void setup_tcp_server(int *sockfd, struct sockaddr_in *addr, in_addr_t address, const socklen_t *addr_len)
{
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    addr->sin_family = AF_INET;
    addr->sin_addr   = *(struct in_addr *)&address;
    addr->sin_port   = 0;

    if(bind(*sockfd, (struct sockaddr *)addr, *addr_len) == -1)
    {
        perror("bind");
        close(*sockfd);
        exit(EXIT_FAILURE);
    }
}
