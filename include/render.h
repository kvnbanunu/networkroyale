#ifndef RENDER_H
#define RENDER_H

#include "../include/game.h"
#include <ncurses.h>

#define N_WINDOWS 3

typedef enum Player_Color
{
    PLAYER_CHAR,
    ENEMY_CHAR,
    NON_CHAR,
    DEAD
}p_color;

void r_setup(WINDOW *windows[N_WINDOWS]);
void r_init(player_t players[MAX_PLAYERS], WINDOW *windows[N_WINDOWS], int player);

#endif // !DEBUG
