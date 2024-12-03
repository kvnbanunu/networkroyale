#include "../include/game.h"
#include <unistd.h>
#include <stdio.h>

int main()
{
    uint8_t player_info[INFO_LEN];
    in_port_t port = 0;

    join_game(player_info, port);

    return 0;
}
