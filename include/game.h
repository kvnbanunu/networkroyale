#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdlib.h>

#define TITLE_LEN 8
#define NUM_CLASSES 9
#define MAX_PLAYERS 25
#define NAME_LEN 8

typedef struct Class_Base
{
    int id;
    int hp;
    int atk;
    int dmg_mod;
    int def;
    int eva;
    int crit_rate;
} class_t;

typedef struct Coordinate
{
    uint8_t x;
    uint8_t y;
} coord_t;

typedef struct Player // 18 bytes
{
    uint8_t id;
    uint8_t is_alive;
    uint8_t class_type;
    uint8_t has_skill;
    uint8_t active_skill;
    uint8_t hp;
    uint8_t exp;
    coord_t pos;
    uint8_t username[NAME_LEN]; // 8 bytes
} player_t;

typedef struct Event_Node
{
    uint8_t actor;
    uint8_t target;
    uint8_t dmg;
    uint8_t dodge;
    uint8_t death;
    uint8_t skill_use;
    uint8_t jobadv;
    struct Event_Node *next;
} *event_t;

enum Class_ID
{
    MOB,
    CLERIC,
    FIGHTER,
    MAGE,
    ROGUE,
    PALADIN,
    KNIGHT,
    WIZARD,
    ASSASSIN
};

void send_position();
void receive_position();
void check_inbound();
void check_collision();
void battle();

#endif // GAME_H
