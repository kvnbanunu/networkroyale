#include "../include/getaddress.h"
#include "../include/args.h"
#include "../include/socket.h"
#include "../include/usage.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>

int main()
{
    char                   *address = NULL;
    char                   *port_str = NULL;
    int                     sockfd;
    in_port_t               port;
    usage_func              usage = usage_s;
    struct sockaddr_storage addr;

    getaddress(&address);
    convert_address(address, &addr);
    sockfd = socket_create(addr.ss_family, SOCK_DGRAM, 0);
    socket_bind(sockfd, &addr, 0);

    socket_close(sockfd);

    return EXIT_SUCCESS;
}
