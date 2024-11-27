#ifndef CLASS_TYPES_H
#define CLASS_TYPES_H

#include <stdint.h>

#define TITLE_LEN 8
#define NUM_CLASSES 4

typedef struct ClassBase
{
    int id;
    int hp;
    int atk;
    int def;
    int eva;
    int crit_rate;
} class_t;

enum Class_List
{
    CLERIC,
    FIGHTER,
    MAGE,
    ROGUE,
    PALADIN,
    KNIGHT,
    WIZARD,
    ASSASSIN
};

#endif // CLASS_TYPES_H


