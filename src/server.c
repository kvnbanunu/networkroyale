#include "../include/setup.h"
#include "../include/socket.h"
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
    int                sockfd;
    int                retval      = EXIT_FAILURE;
    int                num_players = 0;
    char               address_str[INET_ADDRSTRLEN];
    struct sockaddr_in addr;
    in_addr_t          address;
    in_port_t          port     = 0;    // System chooses port
    socklen_t          addr_len = sizeof(addr);

    setup_signal_handler();

    // Get the first IPv4 address of this system that matches 192.168.*.*
    findaddress(&address, address_str);

    setup_tcp_server(&sockfd, &addr, address, &addr_len);

    // Retrieve the port since we passed in zero
    if(getsockname(sockfd, (struct sockaddr *)&addr, &addr_len) == -1)
    {
        perror("getsockname");
        goto done;
    }
    port = addr.sin_port;

    if(listen(sockfd, MAX_PLAYERS) == -1)
    {
        perror("listen");
        goto done;
    }

    printf("Server waiting for players on %s : %d\n", address_str, ntohs(port));

    while(!exit_flag && num_players != MAX_PLAYERS)
    {
        printf("Player %d has joined the lobby\n", ++num_players);
        sleep(1);
    }

    retval = EXIT_SUCCESS;

done:
    socket_close(sockfd);
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
