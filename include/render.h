#ifndef RENDER_H
#define RENDER_H

#include "game.h"
#include <ncurses.h>

#define N_WINDOWS 3
#define MAX_EVENT_BUF 204

typedef enum Player_Color
{
    PLAYER_CHAR = 1,
    ENEMY_CHAR,
    NON_CHAR,
    DEAD
}p_color;

void r_setup(WINDOW *windows[N_WINDOWS]);
void r_init(player_t players[MAX_PLAYERS], WINDOW *windows[N_WINDOWS], int player);
void r_update(player_t players[MAX_PLAYERS], WINDOW *windows[N_WINDOWS], int player, uint8_t buf[PACK_LEN]);

#endif // !DEBUG
