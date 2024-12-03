#include "../include/setup.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PREFIX "192.168"

/* address saved as in network endiannes */
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

/* Creates socket, sets addr fields, binds socket, updates the addr, returns port as network endian. */
in_port_t setup_and_bind(int *sockfd, struct sockaddr_in *addr, in_addr_t address, socklen_t addr_len, int type, int flag)
{
    *sockfd = socket(AF_INET, type, 0);
    if(*sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if(flag != 0)
    {
        int flags = fcntl(*sockfd, F_GETFL, 0);
        if(flags < 0)
        {
            perror("fcntl(F_GETFL)");
            close(*sockfd);
            exit(EXIT_FAILURE);
        }

        if(fcntl(*sockfd, F_SETFL, flags | flag) < 0)
        {
            perror("fcntl(F_GETFL)");
            close(*sockfd);
            exit(EXIT_FAILURE);
        }
    }

    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr   = *(struct in_addr *)&address;
    addr->sin_port   = 0;

    if(bind(*sockfd, (struct sockaddr *)addr, addr_len) == -1)
    {
        perror("bind");
        close(*sockfd);
        exit(EXIT_FAILURE);
    }

    // Update the addr with
    if(getsockname(*sockfd, (struct sockaddr *)addr, &addr_len) == -1)
    {
        perror("getsockname");
        close(*sockfd);
        exit(EXIT_FAILURE);
    }
    return addr->sin_port;
}

void setup_and_connect(int *sockfd, struct sockaddr_in *addr, const char *address, in_port_t port, socklen_t addr_len)
{
    memset(addr, 0, sizeof(*addr));
    if(inet_pton(AF_INET, address, &addr->sin_addr) == -1)
    {
        perror("inet_pton");
        exit(EXIT_FAILURE);
    }

    addr->sin_family = AF_INET;
    addr->sin_port   = htons(port);

    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(*sockfd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if(connect(*sockfd, (struct sockaddr *)addr, addr_len) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }
}

void socket_close(int sockfd)
{
    if(close(sockfd) == -1)
    {
        perror("Error closing socket");
        exit(EXIT_FAILURE);
    }
}
