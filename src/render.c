#include "../include/render.h"
#include <ncurses.h>
#include <stdint.h>
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
    refresh();
}

static void display_stats(WINDOW **window, player_t p)
{
    wclear(*window);
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

static void clear_players(player_t players[MAX_PLAYERS], WINDOW **win)
{
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        player_t p = players[i];
        if(p.is_alive == 0)
        {
            continue;
        }
        mvwaddch(*win, p.pos.y + 1, p.pos.x + 1, ' ');
    }
}

static void display_players(player_t players[MAX_PLAYERS], WINDOW **win, int player)
{
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        player_t p      = players[i];
        if(p.is_alive == 0)
        {
            mvwaddch(*win, p.pos.y + 1, p.pos.x + 1, '%' | COLOR_PAIR(DEAD));
            continue;
        }
        int      color  = (p.class_type != MOB) ? ENEMY_CHAR : NON_CHAR;
        chtype   sprite = sprite_list[p.class_type];
        mvwaddch(*win, p.pos.y + 1, p.pos.x + 1, sprite | COLOR_PAIR(color));
    }
    // Set this client's sprite to the special one
    mvwaddch(*win, players[player].pos.y + 1, players[player].pos.x + 1, '@' | COLOR_PAIR(PLAYER_CHAR));
    wrefresh(*win);
}

static void class_up(player_t *p, int class_type)
{
    const class_t *c = get_class_data(class_type);
    p->class_type = class_type;
    p->hp = c->hp;
    p->has_skill = 1;
}

#define E_START 100
#define E_SIZE 7

static void process_events(WINDOW **win, player_t players[], uint16_t events[], uint16_t num_events, int playerid)
{
    wclear(*win);
    int num_displayed = 1;
    for(int i = 0; i < E_SIZE * num_events; i += E_SIZE)
    {
        int tracker = 1;
        player_t *actor = &players[events[i] - 1];
        player_t *target = &players[events[i + tracker++] - 1];
        if(events[tracker++] != 0) // dmg
        {
            uint16_t dmg = events[tracker - 1];
            mvwprintw(*win, num_displayed++, 1, "%s hit %s for %u", actor->username, target->username, dmg);
            actor->active_skill = 0;
            if(target->id == playerid)
            {
                target->hp = (target->hp - dmg < 0) ? 0 : target->hp - dmg;
            }
        }
        if(events[tracker++] != 0) // dodge
        {
            mvwprintw(*win, num_displayed++, 1, "%s dodged %s's attack!", actor->username, target->username);
        }
        if(events[tracker++] != 0) // death
        {
            mvwprintw(*win, num_displayed++, 1, "%s has been slain", actor->username);
            actor->is_alive = 0;
            target->exp++;
        }
        if(events[tracker++] != 0) // skill activate
        {
            const class_t *c = get_class_data(actor->class_type);
            switch(actor->class_type)
            {
                case CLERIC:
                    mvwprintw(*win, num_displayed++, 1, "%s has healed to full health!", actor->username);
                    break;
                case FIGHTER:
                    mvwprintw(*win, num_displayed++, 1, "%s is preparing for a double attack", actor->username);
                    break;
                case MAGE:
                    mvwprintw(*win, num_displayed++, 1, "%s has teleported!", actor->username);
                    break;
                case ROGUE:
                    mvwprintw(*win, num_displayed++, 1, "%s has disappeared??", actor->username);
                    break;
                case PALADIN:
                    mvwprintw(*win, num_displayed++, 1, "%s has healed to full health!", actor->username);
                    break;
                case KNIGHT:
                    mvwprintw(*win, num_displayed++, 1, "%s is readying their blade for a decisive strike", actor->username);
                    break;
                case WIZARD:
                    mvwprintw(*win, num_displayed++, 1, "%s has teleported!", actor->username);
                    break;
                case ASSASSIN:
                    mvwprintw(*win, num_displayed++, 1, "%s has gone invisible??", actor->username);
                    break;
                default:
                    break;
            }
            actor->active_skill = c->skill_duration;
            actor->has_skill = 0;
        }
        if(events[tracker++] != 0) // job advance
        {
            switch(actor->class_type)
            {
                case CLERIC:
                    mvwprintw(*win, num_displayed++, 1, "%s has advanced from a Cleric to a Paladin!", actor->username);
                    class_up(actor, PALADIN);
                    break;
                case FIGHTER:
                    mvwprintw(*win, num_displayed++, 1, "%s has advanced from a Fighter to Knight!", actor->username);
                    class_up(actor, KNIGHT);
                    break;
                case MAGE:
                    mvwprintw(*win, num_displayed++, 1, "%s has advanced from a Mage to a Wizard!", actor->username);
                    class_up(actor, WIZARD);
                    break;
                case ROGUE:
                    mvwprintw(*win, num_displayed++, 1, "%s has advanced from a Rogue to an Assassin!", actor->username);
                    class_up(actor, ASSASSIN);
                    break;
                default:
                    break;
            }
        }
    }
    wrefresh(*win);
}

static int unpack_events(uint16_t events[MAX_EVENT_BUF], uint8_t buf[PACK_LEN])
{
    int result = 0;
    int tracker = 0;
    int events_added = 0;
    for(int i = E_START; i < PACK_LEN; i += 2)
    {
        uint16_t net_e;
        uint16_t home_e;
        memcpy(&net_e, buf + i, sizeof(uint16_t));
        home_e = ntohs(net_e);
        if(tracker == 0 && home_e == 0) // when tracker is 0, the actor should be extracted which won't be zero this signals the end
        {
            break;
        }
        tracker++;
        if(tracker == E_SIZE)
        {
            result++;
            tracker = 0;
        }
        memcpy(events + events_added, &home_e, sizeof(uint16_t));
        events_added++;
    }
    return result;
}

static void unpack_positions(player_t players[MAX_PLAYERS], uint8_t buf[PACK_LEN])
{
    int dest = (int)(sizeof(uint16_t) * 2);
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        player_t *p = &players[i];
        uint16_t x;
        uint16_t y;
        memcpy(&x, buf + (i*dest), sizeof(uint16_t));
        memcpy(&y, buf + (i*dest) + sizeof(uint16_t), sizeof(uint16_t));
        p->pos.x = ntohs(x);
        p->pos.y = ntohs(y);
        printf("p:%d (%u,%u)\n", i, p->pos.x, p->pos.y);
    }
}

void r_init(player_t players[MAX_PLAYERS], WINDOW *windows[N_WINDOWS], int player)
{
    display_players(players, &windows[0], player);
    display_stats(&windows[1], players[player]);
    wrefresh(windows[2]);
}

static void reduce_skill_duration(player_t players[MAX_PLAYERS])
{
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        player_t *p = &players[i];
        if(p->active_skill == 0)
        {
            continue;
        }
        p->active_skill--;
    }
}

void r_update(player_t players[MAX_PLAYERS], WINDOW *windows[N_WINDOWS], int player, uint8_t buf[PACK_LEN])
{
    int num_events;
    uint16_t events[MAX_EVENT_BUF];
    memset(events, 0, MAX_EVENT_BUF);
    clear_players(players, &windows[0]);
    unpack_positions(players, buf);
    num_events = unpack_events(events, buf);
    process_events(&windows[2], players, events, num_events, player);
    reduce_skill_duration(players);
    display_players(players, &windows[0], player);
    display_stats(&windows[1], players[player]);
    memset(buf, 0, PACK_LEN);
}
