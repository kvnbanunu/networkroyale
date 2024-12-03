#include "../include/game.h"
#include "../include/setup.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define MAP_BOUNDS 255    // Inclusive
#define INFO_LEN 14
#define LISTEN_TIMEOUT 5
#define INPUT_SIZE 6
#define INIT_BOARD_BUF_LEN 400

static volatile sig_atomic_t exit_flag = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

typedef struct ClientInfo
{
    int                socket;
    struct sockaddr_in addr;
} client_info_t;

typedef struct ThreadData
{
    int           serverfd;
    int           player_count;
    client_info_t clients[MAX_PLAYERS];
    player_t      players[MAX_PLAYERS];
    struct pollfd fds[MAX_PLAYERS + 2];
    nfds_t        nfds;
    in_port_t     udp_port;
} thread_data_t;

static void *handle_new_player(void *arg);
static void  init_mobs(thread_data_t *data);
static void  send_initial_board(thread_data_t *data);
static void  receive_input(input_t inputs[], int sockfd);
static void  fill_random_moves(input_t inputs[], player_t players[], int end);
static void  setup_signal_handler(void);
static void  sig_handler(int sig);

int main(void)
{
    int                udpfd;
    int                retval        = EXIT_FAILURE;
    int                alive_players = MAX_PLAYERS;
    int                game_map[MAP_BOUNDS + 1][MAP_BOUNDS + 1];    // This will hold player ids offset by 1
    char               address_str[INET_ADDRSTRLEN];
    uint8_t            outbound_buf[PACK_LEN];
    uint32_t           seed;
    struct sockaddr_in tcp_addr;
    struct sockaddr_in udp_addr;
    in_addr_t          address;
    in_port_t          tcp_port;
    socklen_t          addr_len = sizeof(struct sockaddr);
    thread_data_t      thread_data;
    pthread_t          thread;
    input_t            inputs[MAX_PLAYERS];
    pid_t              pid;
    event_t           *event_head = NULL;

    setup_signal_handler();

    seed = (uint32_t)time(NULL);
    srand(seed);

    // Get the first IPv4 address of this system that matches 192.168.*.*
    findaddress(&address, address_str);

    memset(&thread_data, 0, sizeof(thread_data));
    thread_data.player_count = -1;

    tcp_port             = setup_and_bind(&(thread_data.serverfd), &tcp_addr, address, addr_len, SOCK_STREAM, 0);
    thread_data.udp_port = setup_and_bind(&udpfd, &udp_addr, address, addr_len, SOCK_DGRAM, O_NONBLOCK);

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
                        const char *start_message = "Game starting...\n";
                        write(STDOUT_FILENO, start_message, strlen(start_message));
                        goto game_start;
                    }
                }
            }
        }
    }
game_start:
    // Send clients the initial state of the game and close all tcp sockets
    init_mobs(&thread_data);
    init_positions(thread_data.players, MAX_PLAYERS, game_map);
    send_initial_board(&thread_data);
    for(int i = 0; i <= thread_data.player_count; i++)
    {
        close(thread_data.clients[i].socket);
    }
    socket_close(thread_data.serverfd);
    thread_data.serverfd = -1;

    event_head = (event_t *)malloc(sizeof(event_t));
    if(event_head == NULL)
    {
        perror("malloc event_head");
        goto done;
    }
    event_head->next = NULL;

    while(!exit_flag && alive_players > 1)
    {
        memset(inputs, 0, MAX_PLAYERS * sizeof(input_t));
        receive_input(inputs, udpfd);
        fill_random_moves(inputs, thread_data.players, MAX_PLAYERS);
        alive_players -= process_inputs(&event_head, thread_data.players, inputs, game_map);
        serialize(outbound_buf, thread_data.players, MAX_PLAYERS, &event_head);
        for(int i = 0; i < thread_data.player_count; i++)
        {
            pid = fork();
            switch(pid)
            {
                case -1:
                    fprintf(stderr, "Error Forking for player: %d\n", i);
                    goto done;
                case 0:
                    sendto(udpfd, outbound_buf, PACK_LEN, 0, (struct sockaddr *)&(thread_data.clients[i].addr), addr_len);
                    break;
                default:
                    continue;
            }
        }
    }
    if(alive_players == 1)
    {
        retval = EXIT_SUCCESS;
    }

done:
    free(event_head);
    socket_close(udpfd);
    return retval;
}

static void *handle_new_player(void *arg)
{
    thread_data_t     *info  = (thread_data_t *)arg;
    int                count = info->player_count;
    struct sockaddr_in player_addr;
    socklen_t          addr_len;
    int                playerfd;
    uint8_t            buf[INFO_LEN];
    in_port_t          player_port;
    char               client_host[NI_MAXHOST];
    player_t          *player = &(info->players[count]);
    uint8_t            response[sizeof(in_port_t) * 2];    // will hold port and playerid (as short)
    uint16_t           net_count;
    uint32_t           net_class;
    const class_t     *class_data = NULL;

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
    // PORT[2], CLASS[4], USERNAME[8]
    read(playerfd, buf, INFO_LEN);

    // Deserialize info from new client
    memcpy(&net_class, buf, sizeof(uint32_t));
    player->class_type = (int)(ntohl(net_class));
    memcpy(&player_port, &buf[(int)sizeof(uint32_t)], sizeof(in_port_t));    // Keep port in network order
    memset(player->username, '\0', NAME_LEN);
    memcpy(player->username, &buf[INFO_LEN - NAME_LEN], NAME_LEN);

    player->id       = (uint8_t)count;
    player->is_alive = 1;

    class_data                  = get_class_data((int)(ntohl(net_class)));
    player->hp                  = class_data->hp;
    info->clients[count].socket = playerfd;

    getnameinfo((struct sockaddr *)&player_addr, addr_len, client_host, NI_MAXHOST, NULL, 0, 0);

    info->clients[count].addr          = player_addr;
    info->clients[count].addr.sin_port = player_port;

    // add player socket to poll
    info->fds[info->nfds].fd     = playerfd;
    info->fds[info->nfds].events = POLLIN;
    info->nfds++;

    // send udp port & player id
    net_count = htons((uint16_t)count);
    memset(response, '\0', sizeof(in_port_t) * 2);
    memcpy(response, &(info->udp_port), sizeof(in_port_t));
    memcpy(&response[sizeof(in_port_t)], &net_count, sizeof(uint16_t));
    write(playerfd, &response, sizeof(in_port_t) * 2);

    printf("Player %s joined the lobby from %s:%d\n", info->players[count].username, client_host, ntohs(info->clients[count].addr.sin_port));
    return NULL;
}

// Set all non-player character settings to a mob class
static void init_mobs(thread_data_t *data)
{
    const class_t *mob_class = get_class_data(MOB);
    const char    *name      = "a Virus";
    for(int i = data->player_count + 1; i < MAX_PLAYERS; i++)
    {
        player_t *mob   = &(data->players[i]);
        mob->id         = i;
        mob->is_alive   = 1;
        mob->class_type = MOB;
        mob->hp         = mob_class->hp;
        memset(mob->username, '\0', NAME_LEN);
        memcpy(mob->username, name, strlen(name));
    }
}

static void send_initial_board(thread_data_t *data)
{
    uint8_t buf[INIT_BOARD_BUF_LEN];
    int     dest = 0;
    // Need to send class_types, position, and username;
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        player_t p         = data->players[i];
        uint32_t net_class = htonl((uint32_t)(p.class_type));
        uint16_t x         = htons(p.pos.x);
        uint16_t y         = htons(p.pos.y);
        memcpy(&buf[dest], &net_class, sizeof(uint32_t));
        dest += (int)(sizeof(uint32_t));
        memcpy(&buf[dest], &x, sizeof(uint16_t));
        dest += (int)(sizeof(uint16_t));
        memcpy(&buf[dest], &y, sizeof(uint16_t));
        dest += (int)(sizeof(uint16_t));
        memcpy(&buf[dest], p.username, NAME_LEN);
        dest += NAME_LEN;
    }

    // send the data to all players (player_count is 0 based)
    for(int i = 0; i <= data->player_count; i++)
    {
        write(data->clients[i].socket, buf, INIT_BOARD_BUF_LEN);
    }
}

static void receive_input(input_t inputs[], int sockfd)
{
    fd_set         readfds;
    struct timeval timeout;
    uint8_t        buf[INPUT_SIZE];    // id, input

    timeout.tv_sec  = LISTEN_TIMEOUT;
    timeout.tv_usec = 0;

    while(1)
    {
        int activity;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);

        activity = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if(activity < 0)
        {
            perror("select");
            break;
        }
        if(activity == 0)    // Timeout
        {
            break;
        }

        if(FD_ISSET(sockfd, &readfds))
        {
            uint16_t id;
            uint16_t meta;
            uint32_t net_input;
            uint32_t input;
            memset(buf, 0, INPUT_SIZE);
            if(recv(sockfd, buf, INPUT_SIZE, 0) < 0)
            {
                perror("recv error");
                break;
            }
            memcpy(&meta, buf, sizeof(uint16_t));
            memcpy(&net_input, &buf[2], sizeof(uint32_t));
            id               = ntohs(meta);
            input            = ntohl(net_input);
            inputs[id].meta  = 1;
            inputs[id].input = (input_e)input;
        }
    }
}

static void fill_random_moves(input_t inputs[], player_t players[], int end)
{
    for(int i = 0; i < end; i++)
    {
        if(inputs[i].meta == 0 && players[i].is_alive)
        {
            inputs[i].meta  = 1;
            inputs[i].input = (input_e)(rand() % 3);    // Movement only
        }
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
