#include "../include/getaddress.h"
#include "../include/socket.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

int main(void)
{
    char     *address;
    int       sockfd;
    in_port_t port = 0; // System chooses port
    // usage_func              usage = usage_s;
    struct sockaddr_storage addr;
    socklen_t               addr_len = sizeof(addr);

    // Get the first IPv4 address of this system that matches 192.168.*.*
    address = (char *)malloc(INET_ADDRSTRLEN);
    getaddress(&address);

    convert_address(address, &addr);
    sockfd = socket_create(addr.ss_family, SOCK_DGRAM, 0);
    socket_bind(sockfd, &addr, port);

    // Retrieve the port since we passed in zero
    getsockname(sockfd, (struct sockaddr *)&addr, &addr_len);
    port = ((struct sockaddr_in *)&addr)->sin_port;

    printf("IP: %s\n", address);
    printf("Port: %d\n", ntohs(port));
    socket_close(sockfd);
    free(address);

    return EXIT_SUCCESS;
}
