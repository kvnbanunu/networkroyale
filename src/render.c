#include "../include/render.h"
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STAT_W 24
#define STAT_L 12

// aligns with class_list
static const chtype sprite_list[] = {'v', 'C', 'F', 'M', 'R', 'P', 'K', 'W', 'A'};

// static const char skill_list[NUM_CLASSES][] = {"", "Full Heal", "Double Hit", "Teleport", "Invisibility", "Full Heal", "Double Hit", "Teleport", "Invisibility"};

void r_setup(WINDOW *windows[N_WINDOWS])
{
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

    windows[0] = newwin(MAP_L + 2, MAP_W + 2, 1, 1);
    windows[1] = newwin(STAT_L + 2, STAT_W + 2, MAP_L/2, MAP_W + 2 + STAT_W/2);
    windows[2] = newwin(MAP_L, MAP_W + 2 + STAT_W, 2 + MAP_L + 2, 1);

    box(stdscr, 0, 0);
    for(int i = 0; i < N_WINDOWS; i++)
    {
        box(windows[i], 0, 0);
    }
}

static void display_stats(WINDOW **window, player_t p)
{
    const class_t *c = get_class_data(p.class_type);
    char name[NAME_LEN + 1];
    int count = 1;
    memset(name, '\0', NAME_LEN + 1);
    memcpy(name, p.username, NAME_LEN);
    mvwprintw(*window, count++, 1, "%s", name);
    mvwprintw(*window, count++, 1, "HP:\t%d", p.hp);
    mvwprintw(*window, count++, 1, "EXP:\t%d", p.exp);
    mvwprintw(*window, count++, 1, "ATK:\tD%d+%d", c->atk, c->dmg_mod);
    mvwprintw(*window, count++, 1, "DEF:\t%d", c->def);
    mvwprintw(*window, count++, 1, "EVA:\t%d", c->eva);
    mvwprintw(*window, count++, 1, "CRIT:\t%d", c->crit_rate);
    mvwprintw(*window, count++, 1, "SKILL USES:\t%d", p.has_skill);
    mvwprintw(*window, count++, 1, "SKILL DESCRIPTION:");
    mvwprintw(*window, count++, 1, "%s", c->skill_description);

    wrefresh(*window);
}

void r_init(player_t players[MAX_PLAYERS], WINDOW *windows[N_WINDOWS], int player)
{
    WINDOW  *main  = windows[0];
    WINDOW  *stats = windows[1];
    player_t mc    = players[player];
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        player_t p      = players[i];
        int      color  = (p.class_type != MOB) ? ENEMY_CHAR : NON_CHAR;
        chtype   sprite = sprite_list[p.class_type];
        mvwaddch(main, p.pos.y + 1, p.pos.x + 1, sprite | COLOR_PAIR(color));
    }
    // Set this client's sprite to the special one
    mvwaddch(main, mc.pos.y + 1, mc.pos.x + 1, '@' | COLOR_PAIR(PLAYER_CHAR));
    refresh();
    wrefresh(main);
    wrefresh(windows[2]);
    display_stats(&stats, mc);
}
