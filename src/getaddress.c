#include <arpa/inet.h>
#include <ifaddrs.h>
#include <limits.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PREFIX "192.168"

void getaddress(char **address)
{
    struct ifaddrs       *ifaddr;
    const struct ifaddrs *ifa;
    char                  host[INET_ADDRSTRLEN];

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
            if(getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST) != 0)
            {
                perror("getnameinfo");
                continue;
            }
            if(strncmp(host, PREFIX, strlen(PREFIX)) == 0)
            {
                printf("IP: %s\n", host);
                break;
            }
        }
    }
    freeifaddrs(ifaddr);

    *address = host;
}
