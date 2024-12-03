#include "../include/args.h"
#include "../include/game.h"
#include "../include/render.h"
#include "../include/setup.h"
#include <arpa/inet.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

uint16_t handle_response(int sockfd, in_port_t *port);
void     receive_initial_board(int sockfd, player_t players[MAX_PLAYERS]);

int main(int argc, char *argv[])
{
    int                serverfd;
    int                udpfd;
    int                retval             = EXIT_FAILURE;
    char              *server_address_str = NULL;
    char              *server_port_str    = NULL;
    char               address_str[INET_ADDRSTRLEN];
    uint8_t            player_info[INFO_LEN];
    in_addr_t          address;
    in_port_t          server_port;
    in_port_t          udp_port;
    in_port_t          server_udp_port;
    uint16_t           player_id;
    struct sockaddr_in server_addr;
    struct sockaddr_in udp_addr;
    socklen_t          addr_len = sizeof(struct sockaddr);
    player_t           players[MAX_PLAYERS];
    WINDOW            *windows[N_WINDOWS];

    parse_args(argc, argv, &server_address_str, &server_port_str);
    handle_args(argv[0], server_address_str, server_port_str, &server_port);

    findaddress(&address, address_str);
    udp_port = setup_and_bind(&udpfd, &udp_addr, address, addr_len, SOCK_DGRAM, 0);

    join_game(player_info, udp_port);

    setup_and_connect(&serverfd, &server_addr, server_address_str, server_port, addr_len);

    write(serverfd, player_info, INFO_LEN);

    player_id = handle_response(serverfd, &server_udp_port);

    receive_initial_board(serverfd, players);

    socket_close(serverfd);

    //------------------------------GAME STARTS HERE-----------------------------------------------
    //------------------------------INITIAL RENDER-----------------------------------------------
    r_setup(windows);
    r_init(players, windows, player_id);
    getch();
    //------------------------------GAME LOOP-----------------------------------------------

    for(int i = 0; i < N_WINDOWS; i++)
    {
        delwin(windows[i]);
    }
    endwin();
    //-----------------------------------------------------------------------------
    retval = EXIT_SUCCESS;
    socket_close(udpfd);
    return retval;
}

uint16_t handle_response(int sockfd, in_port_t *port)
{
    uint8_t  response[sizeof(in_port_t) * 2];
    uint16_t player_id;
    read(sockfd, response, sizeof(in_port_t) * 2);
    memcpy(port, response, sizeof(in_port_t));
    memcpy(&player_id, &response[sizeof(in_port_t)], sizeof(uint16_t));
    return player_id;
}

void receive_initial_board(int sockfd, player_t players[MAX_PLAYERS])
{
    uint8_t buf[INIT_BOARD_BUF_LEN];
    int     dest = 0;

    memset(players, 0, sizeof(player_t) * MAX_PLAYERS);

    read(sockfd, buf, INIT_BOARD_BUF_LEN);

    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        player_t *p = &(players[i]);
        uint32_t  net_class;
        uint16_t  net_x;
        uint16_t  net_y;
        p->id       = i;
        p->is_alive = 1;
        memcpy(&net_class, &buf[dest], sizeof(uint32_t));
        dest += (int)(sizeof(uint32_t));
        memcpy(&net_x, &buf[dest], sizeof(uint16_t));
        dest += (int)(sizeof(uint16_t));
        memcpy(&net_y, &buf[dest], sizeof(uint16_t));
        dest += (int)(sizeof(uint16_t));
        p->class_type = (int)(ntohl(net_class));
        p->pos.x      = ntohs(net_x);
        p->pos.y      = ntohs(net_x);
        memcpy(p->username, &buf[dest], NAME_LEN);
        dest += NAME_LEN;
    }
}
