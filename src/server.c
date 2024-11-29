#include "../include/setup.h"
#include "../include/game.h"
#include <arpa/inet.h>
#include <cstdlib>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define INFO_LEN 10
#define GAME_START_LEN 17
#define GAME_TIMER 5
#define COORDS_BUF_LEN 128 // TODO set this to smt else

static volatile sig_atomic_t exit_flag = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

typedef struct PlayerInfo
{
    int                socket;
    struct sockaddr_in addr;
    player_t data;
} player_info_t;

typedef struct ThreadData
{
    int           serverfd;
    int           player_count;
    player_info_t      players[MAX_PLAYERS];
    struct pollfd fds[MAX_PLAYERS + 2];
    nfds_t        nfds;
    in_port_t     udp_port;
} thread_data_t;

void       *handle_new_player(void *arg);
static void setup_signal_handler(void);
static void sig_handler(int sig);

int main(void)
{
    int                udpfd;
    int                retval = EXIT_FAILURE;
    char               address_str[INET_ADDRSTRLEN];
    const char        *game_start_message = "Game starting...\n";
    struct sockaddr_in tcp_addr;
    struct sockaddr_in udp_addr;
    in_addr_t          address;
    in_port_t          tcp_port;
    socklen_t          addr_len = sizeof(struct sockaddr);
    thread_data_t      thread_data;
    pthread_t          thread;
    event_t event_head;
    input_t inputs[MAX_PLAYERS];

    setup_signal_handler();

    // Get the first IPv4 address of this system that matches 192.168.*.*
    findaddress(&address, address_str);

    memset(&thread_data, 0, sizeof(thread_data));
    thread_data.player_count = -1;

    tcp_port             = setup_and_bind(&(thread_data.serverfd), &tcp_addr, address, addr_len, SOCK_STREAM);
    thread_data.udp_port = setup_and_bind(&udpfd, &udp_addr, address, addr_len, SOCK_DGRAM);

    if(listen(thread_data.serverfd, MAX_PLAYERS) == -1)
    {
        perror("listen");
        goto done;
    }

    printf("Server waiting for players on %s : %d\nPress Enter to start game...\n", address_str, ntohs(tcp_port));

    // Setup to poll the server fd and keyboard input
    thread_data.fds[0].fd     = thread_data.serverfd;
    thread_data.fds[0].events = POLLIN;
    thread_data.fds[1].fd     = STDIN_FILENO;
    thread_data.fds[1].events = POLLIN;
    thread_data.nfds          = 2;
    while(!exit_flag)
    {
        // poll because we want to check for non-blocking keyboard input
        if(poll(thread_data.fds, thread_data.nfds, -1) < 0)
        {
            perror("poll");
            goto done;
        }

        for(nfds_t i = 0; i < thread_data.nfds; i++)
        {
            if(thread_data.fds[i].revents & POLLIN)    // funky way to check
            {
                if(thread_data.fds[i].fd == thread_data.serverfd)
                {
                    // player count is the only important shared variable so no need for mutex here
                    thread_data.player_count++;
                    if(pthread_create(&thread, NULL, handle_new_player, (void *)&thread_data) != 0)
                    {
                        perror("pthread_create");
                        goto done;
                    }

                    pthread_join(thread, NULL);

                    if(thread_data.player_count >= MAX_PLAYERS)
                    {
                        goto game_start;
                    }
                }
                else if(thread_data.fds[i].fd == STDIN_FILENO)
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
    socket_close(thread_data.serverfd);
    thread_data.serverfd = 0;
    
    while(!exit_flag)
    {
        memset(inputs, 0, MAX_PLAYERS * sizeof(input_t));
        event_head = NULL;
        receive_input(inputs);
        process_events(&event_head, inputs);
    }
    

close:
    // closing connections
    for(int i = 0; i < thread_data.player_count; i++)
    {
        write(thread_data.players[i].socket, game_start_message, GAME_START_LEN);
        close(thread_data.players[i].socket);
    }
    retval = EXIT_SUCCESS;

done:
    socket_close(udpfd);
    return retval;
}

void *handle_new_player(void *arg)
{
    thread_data_t     *info = (thread_data_t *)arg;
    struct sockaddr_in player_addr;
    socklen_t          addr_len;
    int                playerfd;
    uint8_t            buf[INFO_LEN];
    in_port_t          playerport;
    char               client_host[NI_MAXHOST];
    player_info_t          *player = &(info->players[info->player_count]);

    if(info->player_count >= MAX_PLAYERS)
    {
        printf("Maximum player count reached.\n");
        pthread_exit(NULL);
    }

    addr_len = sizeof(player_addr);
    playerfd = accept(info->serverfd, (struct sockaddr *)&player_addr, &addr_len);
    if(playerfd == -1)
    {
        perror("accept");
        pthread_exit(NULL);
    }
    read(playerfd, buf, INFO_LEN);
    memcpy(player->data.username, buf, NAME_LEN);
    memcpy(&playerport, &buf[NAME_LEN], sizeof(in_port_t));
    player->data.id     = info->player_count;
    player->socket = playerfd;

    getnameinfo((struct sockaddr *)&player_addr, addr_len, client_host, NI_MAXHOST, NULL, 0, 0);

    player->addr          = player_addr;
    player->addr.sin_port = playerport;

    // add player socket to poll
    info->fds[info->nfds].fd     = playerfd;
    info->fds[info->nfds].events = POLLIN;
    info->nfds++;

    // send udp port
    write(playerfd, &info->udp_port, sizeof(in_port_t));

    printf("Player %s joined the lobby from %s:%d\n", player->data.username, client_host, ntohs(player->addr.sin_port));
    return NULL;
}

static void receive_input(input_t inputs[], int connected_players, int sockfd)
{
    int players_moved = 0;
    time_t start;
    time_t end;
    time(&start);
    end = start + GAME_TIMER;
    while(start <= end && players_moved++ < connected_players && !exit_flag )
    {
        uint8_t buf[]; // TODO fill this
        recv(sockfd, &buf, COORDS_BUF_LEN);
    }
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
