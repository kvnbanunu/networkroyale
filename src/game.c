#include "../include/game.h"
#include <stdlib.h>

#define PERCENT 100

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

static void add_event(event_t *list, event_t *event)
{
    event_t curr = *list;
    
    // This would be the first event
    if (curr == NULL)
    {
        *list = *event;
        return;
    }
    while(curr->next != NULL)
    {
        curr = curr->next;
    }
    curr->next = *event;
}

static void clear_events(event_t *list)
{
    event_t curr = *list;
    event_t to_del = NULL;
    while(curr->next != NULL)
    {
        to_del = curr;
        free(to_del);
        curr = curr->next;
    }
    free(curr);
}

static void init_event(event_t *event, uint8_t actor, uint8_t target, uint8_t dmg, uint8_t dodge, uint8_t death, uint8_t skill, uint8_t jobadv)
{
    (*event)->actor = actor;
    (*event)->target = target;
    (*event)->dmg = dmg;
    (*event)->dodge = dodge;
    (*event)->death = death;
    (*event)->skill_use = skill;
    (*event)->jobadv = jobadv;
    (*event)->next = NULL;
}

// Returns the damage c1 deals to c2, 0 == dodge
static uint8_t dmg_calc(class_t attacker, class_t defender, int a_skill, int d_skill)
{
    int result;
    int evasion_check;
    int crit_roll;
    int atk_dice = attacker.atk;
    int def_mod = defender.def;

    if (d_skill && (defender.id == ROGUE || defender.id == ASSASSIN))
    {
        return 0;
    }

    evasion_check = rand()%PERCENT;
    if (evasion_check < defender.eva)
    {
        return 0;
    }
    
    crit_roll = rand()%PERCENT;
    if (crit_roll <= attacker.crit_rate)
    {
        def_mod = 0;
    }
    
    result = rand()%atk_dice;
    if(a_skill && (attacker.id == FIGHTER || attacker.id == KNIGHT))
    {
        result += rand()%atk_dice;
    }

    return (uint8_t)(result + attacker.dmg_mod - def_mod);
}

static void combat(event_t *list, player_t *p1, player_t *p2, int *alive_players)
{
    player_t *a = p1;
    player_t *d = p2;
    uint8_t dmg;
    event_t event;
    uint8_t dodge_flag = 0;
    int winner = 1;

    // determine combat turn order
    if(p1->class_type < p2->class_type)
    {
        a = p2;
        d = p1;
    }
    dmg = dmg_calc(class_list[a->class_type], class_list[d->class_type], a->active_skill, d->active_skill);
    event = malloc(sizeof(struct Event_Node));
    if (dmg == 0)
    {
        dodge_flag = 1;
    }
    init_event(&event, a->id, d->id, dmg, dodge_flag, 0, 0, 0);
    add_event(list, &event);
    if (d->hp - dmg <= 0) // defender dies
    {
        event_t death = malloc(sizeof(struct Event_Node));
        init_event(&death, d->id, a->id, 0, 0, 1, 0, 0);
        add_event(list, &death);
        d->is_alive = 0;
        d->hp = 0;
        a->exp++;
        *alive_players--;
        return;
    }
    else
    {
        // Defenders turn to deal damage
        event_t atk2 = malloc(sizeof(struct Event_Node));
        d->hp -= dmg;
        dmg = dmg_calc(class_list[d->class_type], class_list[a->class_type], d->active_skill, a->active_skill);
        dodge_flag = (dmg == 0) ? 1 : 0;
        init_event(&atk2, d->id, a->id, dmg, dodge_flag, 0, 0, 0);
        add_event(list, &atk2);
        if (a->hp - dmg <= 0) // attacker dies
        {
            event_t death = malloc(sizeof(struct Event_Node));
            init_event(&death, a->id, d->id, 0, 0, 1, 0, 0);
            add_event(list, &death);
            a->is_alive = 0;
            a->hp = 0;
            d->exp++;
            *alive_players--;
            return;
        }
        a->hp -= dmg;
    }
}


void process_events(event_t *events, player_info_t *players, input_t *inputs, int *alive_players)
{
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        player_t 
        if(!check_inbound())
        {
            continue; // no movement
        }
    }
}
