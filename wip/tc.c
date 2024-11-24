#include "../include/socket.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define ADDRESS "192.168.216.82"
#define NAME_LEN 5
#define BUF_L 64
#define PDATA_L 13
#define PORT 9999

typedef struct Player
{
    uint8_t  name[NAME_LEN];
    uint32_t x;
    uint32_t y;
} player_t;

int main(void)
{
    int fdtcp;
    int fdudp;
    // int                     sfd;
    player_t                player;
    uint32_t                coord;
    uint8_t                 pdata[PDATA_L];
    char                    buffer[BUF_L];
    const char             *message;
    size_t                  pdataind;
    const in_port_t         P1 = PORT;
    const in_port_t         P2 = PORT + 1;
    struct sockaddr_storage addr1;
    struct sockaddr_storage addr2;
    struct sockaddr_storage saddr;
    socklen_t               addr_len = sizeof(saddr);

    convert_address(ADDRESS, &addr1);
    convert_address(ADDRESS, &addr2);

    fdtcp = socket_create(addr1.ss_family, SOCK_STREAM, 0);
    fdudp = socket_create(addr2.ss_family, SOCK_DGRAM, 0);

    // Setup

    message = "Enter a username: ";
    write(1, message, strlen(message));
    memset(&buffer, '\0', BUF_L);
    read(0, &buffer, NAME_LEN);

    // Connect to server
    socket_connect(fdtcp, &addr1, P1);
    write(fdtcp, &buffer, NAME_LEN);
    fprintf(stdout, "\nSending username to server.\n");

    memset(&buffer, '\0', NAME_LEN);
    read(fdtcp, &buffer, BUF_L);
    fprintf(stdout, "%s\n", buffer);
    memset(&buffer, '\0', BUF_L);

    socket_close(fdtcp);

    // UDP
    socket_bind(fdudp, &addr2, P2);
    fprintf(stdout, "Waiting for server update\n");

    recvfrom(fdudp, pdata, PDATA_L, 0, (struct sockaddr *)&addr2, &addr_len);
    printf("Data received: %s %u %u\n", pdata, pdata[NAME_LEN], pdata[NAME_LEN + sizeof(uint32_t)]);

    pdataind = sizeof(player.name);
    memcpy(player.name, &pdata, pdataind);

    memcpy(&coord, &pdata[pdataind], sizeof(uint32_t));
    player.x = ntohl(coord);
    pdataind += sizeof(uint32_t);
    memcpy(&coord, &pdata[pdataind], sizeof(uint32_t));
    player.y = ntohl(coord);

    fprintf(stdout, "Player data: \nname: %s\nx: %u\ny: %u\n", player.name, player.x, player.y);

    socket_close(fdudp);
}
