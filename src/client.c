#include "../include/args.h"
#include "../include/controller.h"
#include "../include/setup.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define NAME_LEN 5
#define INFO_LEN 7
#define GAME_START_LEN 17

int main(int argc, char *argv[])
{
    int                serverfd;
    int                udpfd;
    int                retval             = EXIT_FAILURE;
    char              *server_address_str = NULL;
    char              *server_port_str    = NULL;
    char               address_str[INET_ADDRSTRLEN];
    const char        *message;
    char               username[NAME_LEN];
    uint8_t            player_info[INFO_LEN];
    char               game_start_message[GAME_START_LEN + 1];
    in_addr_t          address;
    in_port_t          server_port;
    in_port_t          udp_port;
    in_port_t          server_udp_port;
    struct sockaddr_in server_addr;
    struct sockaddr_in udp_addr;
    socklen_t          addr_len = sizeof(struct sockaddr);

    parse_args(argc, argv, &server_address_str, &server_port_str);
    handle_args(argv[0], server_address_str, server_port_str, &server_port);

    message = "Enter a username:\n";
    write(1, message, strlen(message) + 1);
    read(1, username, NAME_LEN);

    findaddress(&address, address_str);
    udp_port = setup_and_bind(&udpfd, &udp_addr, address, addr_len, SOCK_DGRAM);

    // serializing player info [name, address, port]
    memset(player_info, '\0', INFO_LEN);
    memcpy(player_info, username, NAME_LEN);
    memcpy(&player_info[NAME_LEN], &udp_port, sizeof(in_port_t));

    setup_and_connect(&serverfd, &server_addr, server_address_str, server_port, addr_len);

    write(serverfd, player_info, INFO_LEN);

    read(serverfd, &server_udp_port, sizeof(in_port_t));

    memset(game_start_message, '\0', GAME_START_LEN + 1);
    read(serverfd, game_start_message, GAME_START_LEN);

    printf("%s", game_start_message);
    socket_close(serverfd);

    // Start the game here
    printf("GOING TO START THE CONTROLLER\n");
    controller();

    retval = EXIT_SUCCESS;

    socket_close(udpfd);
    return retval;
}
