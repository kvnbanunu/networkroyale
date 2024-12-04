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

struct Client_Data
{
    int id;
    int fd;
    int serverfd;
    in_addr_t address;
    in_port_t port;
    in_port_t server_tcp_port;
    in_port_t server_udp_port;
    struct sockaddr_in addr;
    struct sockaddr_in server_addr;
};

void handle_response(struct Client_Data *data);
void     receive_initial_board(int sockfd, player_t players[MAX_PLAYERS]);

int main(int argc, char *argv[])
{
    struct Client_Data c_data = {0};
    int                retval             = EXIT_FAILURE;
    char              *server_address_str = NULL;
    char              *server_port_str    = NULL;
    char               address_str[INET_ADDRSTRLEN];
    uint8_t            player_info[INFO_LEN];
    player_t           players[MAX_PLAYERS];
    WINDOW            *windows[N_WINDOWS];
    socklen_t addr_len = sizeof(struct sockaddr);

    parse_args(argc, argv, &server_address_str, &server_port_str);
    handle_args(argv[0], server_address_str, server_port_str, &(c_data.server_tcp_port));

    findaddress(&(c_data.address), address_str);
    c_data.port = setup_and_bind(&(c_data.fd), &(c_data.addr), c_data.address, addr_len, SOCK_DGRAM, 0);

    join_game(player_info, &(c_data.port));

    setup_and_connect(&(c_data.serverfd), &(c_data.server_addr), server_address_str, c_data.server_tcp_port, addr_len);

    write(c_data.serverfd, player_info, INFO_LEN);

    handle_response(&c_data);

    receive_initial_board(c_data.serverfd, players);

    socket_close(c_data.serverfd);

    //------------------------------GAME STARTS HERE-----------------------------------------------
    //------------------------------INITIAL RENDER-----------------------------------------------
    r_setup(windows);
    r_init(players, windows, c_data.id);
    getch();
    //------------------------------GAME LOOP-----------------------------------------------

    for(int i = 0; i < N_WINDOWS; i++)
    {
        delwin(windows[i]);
    }
    curs_set(1);
    endwin();
    //-----------------------------------------------------------------------------
    retval = EXIT_SUCCESS;
    socket_close(c_data.fd);
    return retval;
}

void handle_response(struct Client_Data *data)
{
    uint8_t  response[sizeof(in_port_t) + sizeof(uint32_t)];
    uint32_t playerid;
    read(data->serverfd, response, sizeof(response));
    memcpy(&(data->server_udp_port), response, sizeof(in_port_t));
    memcpy(&playerid, response + sizeof(in_port_t), sizeof(uint32_t));
    data->id = (int)(ntohl(playerid));

    printf("player id: %u\n", data->id);
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
        p->pos.y      = ntohs(net_y);
        memcpy(p->username, &buf[dest], NAME_LEN);
        dest += NAME_LEN;
    }
}
