#include "../include/args.h"
#include "../include/socket.h"
#include "../include/usage.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>

int main(int argc, char *argv[])
{
    char *address = NULL;
    char *port_str = NULL;
    int sockfd;
    in_port_t port;
    usage_func usage = usage_c;
    struct sockaddr_storage addr;

    parse_args_s(argc, argv, &address, &port_str);
    handle_args_s(argv[0], address, port_str, &port);
    convert_address(address, &addr);
    sockfd = socket_create(addr.ss_family, SOCK_DGRAM, 0);



    socket_close(sockfd);

    return EXIT_SUCCESS;
}
