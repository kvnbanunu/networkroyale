#include "../include/setup.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PLAYERS 25

static volatile sig_atomic_t exit_flag = 0;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void setup_signal_handler(void);
static void sig_handler(int sig);

int main(void)
{
    int                tcpfd;
    int                udpfd;
    int                retval      = EXIT_FAILURE;
    int                num_players = 0;
    int                count       = 0;
    int                players[MAX_PLAYERS];
    char               address_str[INET_ADDRSTRLEN];
    struct sockaddr_in tcp_addr;
    struct sockaddr_in udp_addr;
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

    printf("Server waiting for players on %s : %d\n", address_str, ntohs(tcp_port));
    printf("TCP : %s:%d\n", address_str, ntohs(tcp_port));
    printf("UDP : %s:%d\n", address_str, ntohs(udp_port));

    while(!exit_flag && num_players != MAX_PLAYERS)
    {
        count++;
        players[num_players] = count;
        printf("Player %d has joined the lobby\n", players[num_players++]);
        sleep(1);
    }

    retval = EXIT_SUCCESS;

done:
    socket_close(tcpfd);
    socket_close(udpfd);
    return retval;
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
