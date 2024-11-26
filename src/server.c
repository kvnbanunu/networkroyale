#include "../include/setup.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PLAYERS 25
#define NAME_LEN 5
#define INFO_LEN 7
#define GAME_START_LEN 17

static volatile sig_atomic_t exit_flag = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

typedef struct Player
{
    int                id;
    int                socket;
    struct sockaddr_in addr;
    char               username[NAME_LEN];
} player_t;

void        handle_new_player(int serverfd, struct pollfd *fds, nfds_t *nfds, player_t *player, int *player_count, in_port_t udp_port);
static void setup_signal_handler(void);
static void sig_handler(int sig);

int main(void)
{
    int                tcpfd;
    int                udpfd;
    int                retval       = EXIT_FAILURE;
    int                player_count = 0;
    char               address_str[INET_ADDRSTRLEN];
    const char        *game_start_message = "Game starting...\n";
    struct sockaddr_in tcp_addr;
    struct sockaddr_in udp_addr;
    struct pollfd      fds[MAX_PLAYERS + 2];    // Max players + server + keyboard
    nfds_t             nfds;
    player_t           players[MAX_PLAYERS];
    in_addr_t          address;
    in_port_t          tcp_port;
    in_port_t          udp_port;
    socklen_t          addr_len = sizeof(struct sockaddr);

    setup_signal_handler();

    // Get the first IPv4 address of this system that matches 192.168.*.*
    findaddress(&address, address_str);

    tcp_port = setup_and_bind(&tcpfd, &tcp_addr, address, addr_len, SOCK_STREAM);
    udp_port = setup_and_bind(&udpfd, &udp_addr, address, addr_len, SOCK_DGRAM);

    if(listen(tcpfd, MAX_PLAYERS) == -1)
    {
        perror("listen");
        goto done;
    }

    printf("Server waiting for players on %s : %d\nPress Enter to start game...\n", address_str, ntohs(tcp_port));

    // Setup to poll the server fd and keyboard input
    memset(fds, 0, sizeof(fds));
    fds[0].fd     = tcpfd;
    fds[0].events = POLLIN;
    fds[1].fd     = STDIN_FILENO;
    fds[1].events = POLLIN;
    nfds          = 2;
    memset(players, 0, sizeof(players));
    while(!exit_flag)
    {
        // poll because we want to check for non-blocking keyboard input
        if(poll(fds, nfds, -1) < 0)
        {
            perror("poll");
            goto done;
        }

        for(nfds_t i = 0; i < nfds; i++)
        {
            if(fds[i].revents & POLLIN)    // funky way to check
            {
                if(fds[i].fd == tcpfd)
                {
                    handle_new_player(tcpfd, fds, &nfds, &players[player_count], &player_count, udp_port);
                    if(player_count >= MAX_PLAYERS)
                    {
                        goto game_start;
                    }
                }
                else if(fds[i].fd == STDIN_FILENO)
                {
                    char input;
                    if(read(STDIN_FILENO, &input, 1) < 1)
                    {
                        perror("read");
                        goto done;
                    }
                    if(input)
                    {
                        write(STDOUT_FILENO, game_start_message, GAME_START_LEN);
                        goto game_start;
                    }
                }
            }
        }
    }
game_start:
    // Starting game aka closing connections
    for(int i = 0; i < player_count; i++)
    {
        write(players[i].socket, game_start_message, GAME_START_LEN);
        close(players[i].socket);
    }
    retval = EXIT_SUCCESS;

done:
    socket_close(tcpfd);
    socket_close(udpfd);
    return retval;
}

void handle_new_player(int serverfd, struct pollfd *fds, nfds_t *nfds, player_t *player, int *player_count, in_port_t udp_port)
{
    struct sockaddr_in player_addr;
    socklen_t          addr_len;
    int                playerfd;
    uint8_t            buf[INFO_LEN];
    in_port_t          playerport;
    char               client_host[NI_MAXHOST];

    if(*player_count >= MAX_PLAYERS)
    {
        printf("Maximum player count reached.\n");
        return;
    }

    addr_len = sizeof(player_addr);
    playerfd = accept(serverfd, (struct sockaddr *)&player_addr, &addr_len);
    if(playerfd == -1)
    {
        perror("accept");
        return;
    }

    read(playerfd, buf, INFO_LEN);

    memcpy(player->username, buf, NAME_LEN);
    memcpy(&playerport, &buf[NAME_LEN], sizeof(in_port_t));
    player->id     = *player_count;
    player->socket = playerfd;

    getnameinfo((struct sockaddr *)&player_addr, addr_len, client_host, NI_MAXHOST, NULL, 0, 0);

    player->addr          = player_addr;
    player->addr.sin_port = playerport;

    (*player_count)++;

    // add player socket to poll
    fds[*nfds].fd     = playerfd;
    fds[*nfds].events = POLLIN;
    (*nfds)++;

    // Send udp port
    write(playerfd, &udp_port, sizeof(in_port_t));

    printf("Player %s joined the lobby from %s:%d\n", player->username, client_host, ntohs(player->addr.sin_port));
}

/* Pairs SIGINT with sig_handler */
static void setup_signal_handler(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif
    sa.sa_handler = sig_handler;
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/* Write to stdout a shutdown message and set exit_flag to end while loop in main */
static void sig_handler(int sig)
{
    const char *message = "\nSIGINT received. Server shutting down.\n";
    write(1, message, strlen(message));
    exit_flag = 1;
}

#pragma GCC diagnostic pop
