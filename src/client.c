#include "../include/args.h"
#include "../include/setup.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>

#define NAME_LEN 5

int main(int argc, char *argv[])
{
    int serverfd;
    int udpfd;
    int retval = EXIT_FAILURE;
    char *server_address_str = NULL;
    char *server_port_str = NULL;
    char address_str[INET_ADDRSTRLEN];
    in_addr_t address;
    in_port_t server_port;
    in_port_t udp_port;
    usage_func usage = usage_c;
    struct sockaddr_in server_addr;
    struct sockaddr_in udp_addr;
    socklen_t addr_len = sizeof(struct sockaddr);

    parse_args(argc, argv, &server_address_str, &server_port_str);
    handle_args(argv[0], server_address_str, server_port_str, &server_port);
    setup_and_connect(&serverfd, &server_addr, server_address_str, server_port, addr_len);
    findaddress(&address, address_str);
    udp_port = setup_and_bind(&udpfd, &udp_addr, address, addr_len, SOCK_DGRAM);

    socket_close(serverfd);
    return EXIT_SUCCESS;
}
