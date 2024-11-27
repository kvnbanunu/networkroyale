#ifndef GAME_H
#define GAME_H

#include <stdint.h>

#define MAX_PLAYERS 25
#define NAME_LEN 8

typedef struct Player // 18 bytes
{
    uint8_t id;
    uint8_t is_alive;
    uint8_t class_type;
    uint8_t has_skill;
    uint8_t hp;
    uint8_t exp;
    uint16_t x;
    uint16_t y;
    uint8_t username[NAME_LEN]; // 8 bytes
} player_t;

void send_position();
void receive_position();
void check_inbound();
void check_collision();
void battle();

#endif // GAME_H
