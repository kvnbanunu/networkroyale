#include "../include/socket.h"
#include "../include/usage.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define ADDRESS "192.168.0.72"
#define MAX_CONNECTIONS 5
#define NAME_LEN 5
#define BUF_L 64
#define PDATA_L 13

typedef struct Player {
    uint8_t name[NAME_LEN];
    uint32_t x;
    uint32_t y;
} player_t;

int main(int argc, char *argv[])
{
    int                     fdtcp1;
    int fdtcp2;
    int fdudp;
    int cfd;
    player_t *player;
    uint8_t pdata[PDATA_L];
    size_t pdataind;
    usage_func              usage = usage_s;
    const in_port_t P1 = 9997;
    const in_port_t P2 = 9998;
    const in_port_t P3 = 9999;
    struct sockaddr_storage addr1;
    struct sockaddr_storage addr2;
    struct sockaddr_storage addr3;
    struct sockaddr_storage caddr;
    socklen_t caddr_l = sizeof(caddr);

    convert_address(ADDRESS, &addr1);
    convert_address(ADDRESS, &addr2);
    convert_address(ADDRESS, &addr3);
    
    char buffer[BUF_L];

    
    fdtcp1 = socket_create(addr1.ss_family, SOCK_STREAM, 0);
    fdtcp2 = socket_create(addr2.ss_family, SOCK_STREAM, 0);
    fdudp = socket_create(addr3.ss_family, SOCK_DGRAM, 0);

    // Setup
    socket_bind(fdtcp1, &addr1, P1);
    listen(fdtcp1, MAX_CONNECTIONS);
    cfd = accept(fdtcp1, (struct sockaddr *)&caddr, &caddr_l);
    printf("Player connected. Waiting for name.\n");
    read(cfd, &player->name, NAME_LEN);

    player->x = 0;
    player->y = 0;

    memset(&buffer, '\0', BUF_L);
    snprintf(buffer, BUF_L, "Hello %s! Your initial position is (%d, %d)\n", player->name, player->x, player->y);

    write(cfd, &buffer, BUF_L);
    memset(&buffer, '\0', BUF_L);
    
    socket_close(cfd);
    socket_close(fdtcp1);

    // UDP
    socket_bind(fdudp, &addr3, P3);

    pdataind = 0;
    memcpy(pdata, &player->name, pdataind);
    pdataind = sizeof(player->name);
    memcpy(&pdata[pdataind], &player->x, pdataind);
    pdataind += sizeof(player->x);
    memcpy(&pdata[pdataind], &player->y, pdataind);
    pdataind = 0;

    printf("Player data sent.\n");
    sendto(fdudp, pdata, PDATA_L, 0, (struct sockaddr *)&addr3, caddr_l);
    socket_close(fdudp);

    socket_close(fdtcp2);

    return EXIT_SUCCESS;
}
