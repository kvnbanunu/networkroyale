#ifndef GAME_H
#define GAME_H

#include <stdint.h>
#include <stdlib.h>

#define TITLE_LEN 8
#define NUM_CLASSES 9
#define MAX_PLAYERS 25
#define NAME_LEN 8
#define PACK_LEN 508
#define MAP_BOUNDS 255    // inclusive

typedef struct Class_Base
{
    int id;
    int hp;
    int atk;
    int dmg_mod;
    int def;
    int eva;
    int crit_rate;
    int skill_duration;
} class_t;

typedef enum INPUTS
{
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_SKILL
} input_e;

typedef struct Client_Input
{
    uint16_t meta;
    input_e  input;
} input_t;

typedef struct Coordinate
{
    uint16_t x;
    uint16_t y;
} coord_t;

typedef struct Player
{
    int     id;
    int     is_alive;
    int     class_type;
    int     has_skill;
    int     active_skill;
    int     hp;
    int     exp;
    coord_t pos;
    uint8_t username[NAME_LEN];
} player_t;

typedef struct Event_Node
{
    uint16_t           actor;
    uint16_t           target;
    uint16_t           dmg;
    uint16_t           dodge;
    uint16_t           death;
    uint16_t           skill_use;
    uint16_t           jobadv;
    struct Event_Node *next;
} event_t;

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

void init_positions(player_t players[], int num_players, int game_map[MAP_BOUNDS + 1][MAP_BOUNDS + 1]);
int  process_inputs(event_t **event_head, player_t players[], input_t inputs[], int game_map[MAP_BOUNDS + 1][MAP_BOUNDS + 1]);
void serialize(uint8_t buf[], player_t players[], int player_count, event_t **event_head);
const class_t *get_class_data(int class_type);

#endif    // GAME_H
