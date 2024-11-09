#include "../include/args.h"
#include "../include/socket.h"
#include "../include/usage.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>

int main(int argc, char *argv[])
{
    char                   *address  = NULL;
    char                   *port_str = NULL;
    int                     sockfd;
    in_port_t               port;
    usage_func              usage = usage_s;
    struct sockaddr_storage addr;

    parse_args_s(argc, argv, &address, &port_str, usage);
    handle_args_s(argv[0], address, port_str, &port, usage);
    convert_address(address, &addr);
    sockfd = socket_create(addr.ss_family, SOCK_DGRAM, 0);
    socket_bind(sockfd, &addr, port);

    socket_close(sockfd);

    return EXIT_SUCCESS;
}
