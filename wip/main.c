#include "../include/game.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

int main(void)
{
    srand(time(0));
    const class_t class_list[] = {
        // NPC     hp  atk dmg  def  eva   cr
        {MOB,       5,  3,  2,   1,   0,   5},
        // Starting classes
        {CLERIC,   20,  6,  4,   4,   0,   5},
        {FIGHTER,  15,  6,  3,   3,   0,   5},
        {MAGE,     10,  6,  6,   1,   0,   5},
        {ROGUE,    10,  3,  4,   1,  25,  50},
        // Promoted classes
        {PALADIN,  30,  4,  0,   8,   0,  10},
        {KNIGHT,   16,  5,  0,   3,   0,  10},
        {WIZARD,   10, 20,  0,   5,   0,  10},
        {ASSASSIN,  8,  3,  0,   2,  50, 100}
    };



    return 0;
}
