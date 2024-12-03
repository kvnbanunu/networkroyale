#include "../include/render.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

// aligns with class_list
static const char sprite_list[] = {'v', 'C', 'F', 'M', 'R', 'P', 'K', 'W', 'A'};

void r_setup(WINDOW *windows[N_WINDOWS])
{
    int m = MAP_BOUNDS + 1;
    clear();
    initscr();
    curs_set(0);
    noecho();
    cbreak();
    keypad(stdscr, true);

    if(has_colors() == FALSE)
    {
        endwin();
        perror("Terminal does not support color");
        getch();
        exit(EXIT_FAILURE);
    }

    start_color();
    init_pair(PLAYER_CHAR, COLOR_BLUE, COLOR_BLACK);
    init_pair(ENEMY_CHAR, COLOR_WHITE, COLOR_BLACK);
    init_pair(NON_CHAR, COLOR_GREEN, COLOR_BLACK);
    init_pair(DEAD, COLOR_RED, COLOR_BLACK);
    // Add more colours

    windows[0] = newwin(m + 2, m + 2, 1, 1);
    windows[1] = newwin(m + 2, m + 2, 1, 2 + m + 2);
    windows[2] = newwin(m + 2, (m + 2) * 2 + 2, 2 + m + 2, 1);

    box(stdscr, 0, 0);
    for(int i = 0; i < N_WINDOWS; i++)
    {
        box(windows[i], 0, 0);
    }
}

static void display_stats(WINDOW **window, player_t p)
{
    const class_t *c = get_class_data(p.class_type);
    mvwprintw(*window, 1, 1, "%s\nHP:\t%d\nEXP:\t%d\nATK:\tD%d+%d\nDEF:\t%d\nEVA:\t%d\nCRIT:\t%d\nSKILL USES:\t%d\nSKILL DESCRIPTION:\n%s", p.username, p.hp, p.exp, c->atk, c->dmg_mod, c->def, c->eva, c->crit_rate, p.has_skill, c->skill_description);
}

void r_init(player_t players[MAX_PLAYERS], WINDOW *windows[N_WINDOWS], int player)
{
    WINDOW  *main  = windows[0];
    WINDOW  *stats = windows[1];
    player_t mc    = players[player];
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        player_t p     = players[i];
        int      color = (p.class_type != MOB) ? ENEMY_CHAR : NON_CHAR;
        mvwaddch(main, p.pos.y + 1, p.pos.x + 1, p.class_type | COLOR_PAIR(color));
    }
    // Set this client's sprite to the special one
    mvwaddch(main, mc.pos.y, mc.pos.x, '@' | COLOR_PAIR(PLAYER_CHAR));
    display_stats(&stats, mc);
    refresh();
    for(int i = 0; i < N_WINDOWS; i++)
    {
        wrefresh(windows[i]);
    }
}
