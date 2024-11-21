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
#define MAX_CONNECTIONS 5
#define NAME_LEN 5
#define BUF_L 128
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
    int fdtcp1;
    // int                     fdtcp2;
    int      fdudp;
    int      cfd;
    uint32_t coord;
    uint8_t  netname[NAME_LEN];
    player_t player;
    uint8_t  pdata[PDATA_L];
    char     buffer[BUF_L];
    // int             filler;
    size_t          pdataind;
    const in_port_t P1 = PORT;
    // const in_port_t         P2 = PORT;
    const in_port_t         P3 = PORT + 1;
    struct sockaddr_storage addr1;
    // struct sockaddr_storage addr2;
    struct sockaddr_storage addr3;
    struct sockaddr_storage caddr;
    socklen_t               caddr_l = sizeof(caddr);

    convert_address(ADDRESS, &addr1);
    // convert_address(ADDRESS, &addr2);
    convert_address(ADDRESS, &addr3);

    fdtcp1 = socket_create(addr1.ss_family, SOCK_STREAM, 0);
    // fdtcp2 = socket_create(addr2.ss_family, SOCK_STREAM, 0);
    fdudp = socket_create(addr3.ss_family, SOCK_DGRAM, 0);

    // Setup
    socket_bind(fdtcp1, &addr1, P1);
    listen(fdtcp1, MAX_CONNECTIONS);
    cfd = accept(fdtcp1, (struct sockaddr *)&caddr, &caddr_l);
    printf("Player connected. Waiting for name.\n");

    memset(&netname, '\0', NAME_LEN);
    read(cfd, &netname, NAME_LEN);

    memcpy(player.name, &netname, NAME_LEN);
    player.x = 0;
    player.y = 0;

    memset(&buffer, '\0', NAME_LEN);
    // filler         = snprintf(buffer, BUF_L, "Hello %s! Your initial position is (%ud, %ud)\n", player->name, player->x, player->y);
    // buffer[filler] = '\0';
    snprintf(buffer, BUF_L, "Hello %s! Your initial position is (%u, %u)\n", player.name, player.x, player.y);
    write(cfd, &buffer, BUF_L);
    memset(&buffer, '\0', BUF_L);

    socket_close(cfd);
    socket_close(fdtcp1);

    // UDP
    get_address_to_server(&addr3, P3);

    pdataind = sizeof(player.name);
    memcpy(pdata, &player.name, pdataind);
    coord = htonl(player.x);
    memcpy(&pdata[pdataind], &coord, sizeof(uint32_t));
    pdataind += sizeof(player.x);
    coord = htonl(player.y);
    memcpy(&pdata[pdataind], &coord, sizeof(uint32_t));

    printf("Sending Player data: %s %u %u\n", pdata, pdata[NAME_LEN], pdata[NAME_LEN + sizeof(uint32_t)]);
    sleep(1);
    sendto(fdudp, pdata, PDATA_L, 0, (struct sockaddr *)&addr3, caddr_l);
    socket_close(fdudp);

    // socket_close(fdtcp2);

    return EXIT_SUCCESS;
}
