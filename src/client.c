#include "../include/args.h"
#include "../include/setup.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>

#define NAME_LEN 5
#define INFO_LEN 11

int main(int argc, char *argv[])
{
    int serverfd;
    int udpfd;
    int retval = EXIT_FAILURE;
    char *server_address_str = NULL;
    char *server_port_str = NULL;
    char address_str[INET_ADDRSTRLEN];
    char *message;
    char username[NAME_LEN];
    uint8_t player_info[INFO_LEN];
    in_addr_t address;
    in_port_t server_port;
    in_port_t udp_port;
    usage_func usage = usage_c;
    struct sockaddr_in server_addr;
    struct sockaddr_in udp_addr;
    socklen_t addr_len = sizeof(struct sockaddr);

    parse_args(argc, argv, &server_address_str, &server_port_str);
    handle_args(argv[0], server_address_str, server_port_str, &server_port);

    message = "Enter a username:\n";
    write(1, message, sizeof(message) + 1);
    read(1, username, NAME_LEN);

    findaddress(&address, address_str);
    udp_port = setup_and_bind(&udpfd, &udp_addr, address, addr_len, SOCK_DGRAM);

    // serializing player info [name, address, port]
    memset(player_info, '\0', INFO_LEN);
    memcpy(player_info, username, NAME_LEN);
    memcpy(&player_info[NAME_LEN], &address, sizeof(in_addr_t));
    memcpy(&player_info[NAME_LEN + sizeof(in_addr_t)], &udp_port, sizeof(in_port_t));

    setup_and_connect(&serverfd, &server_addr, server_address_str, server_port, addr_len);

    write(serverfd, player_info, INFO_LEN);

    socket_close(serverfd);
    socket_close(udpfd);
    return EXIT_SUCCESS;
}
