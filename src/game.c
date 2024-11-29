#include "../include/game.h"
#include <stdlib.h>

#define PERCENT 100

const class_t class_list[] = {
    // NPC     hp  atk dmg  def  eva   cr skill duration
    {MOB,       5,  3,  2,   1,   0,   5,   0},
    // Starting classes
    {CLERIC,   20,  6,  4,   4,   0,   5,   0},
    {FIGHTER,  15,  6,  3,   3,   0,   5,   5},
    {MAGE,     10,  6,  6,   1,   0,   5,   0},
    {ROGUE,    10,  3,  4,   1,  25,  50,   5},
    // Promoted classes
    {PALADIN,  30,  4,  0,   8,   0,  10,   0},
    {KNIGHT,   16,  5,  0,   3,   0,  10,  10},
    {WIZARD,   10, 20,  0,   5,   0,  10,   0},
    {ASSASSIN,  8,  3,  0,   2,  50, 100,  10}
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

// Returns the amount of players slain this combat turn
static int combat(event_t *list, player_t *p1, player_t *p2)
{
    player_t *a = p1;
    player_t *d = p2;
    uint8_t dmg;
    event_t event;
    uint8_t dodge_flag = 0;

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
        return 1;
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
            return 1;
        }
        a->hp -= dmg;
    }
    return 0;
}

static int check_inbounds(coord_t coord, enum INPUTS input, int outerbound)
{
    switch (input)
    {
        case INPUT_UP:
            return (coord.y - 1 >= 0);
        case INPUT_DOWN:
            return (coord.y + 1 <= outerbound);
        case INPUT_LEFT:
            return (coord.x - 1 >= 0);
        case INPUT_RIGHT:
            return (coord.x + 1 <= outerbound);
        default:
            return 1; // No movement so should be inbound
    }
}

// If the (coord + input) on the map has a player on it, return the id(+1) or 0
static int check_collision(coord_t coord, enum INPUTS input, int outerbound, int *game_map)
{
    int x = (int)coord.x;
    int y = (int)coord.y;
    switch(input)
    {
        case INPUT_UP:
            return *((game_map + x * (outerbound + 1) + (y - 1)));
        case INPUT_DOWN:
            return *((game_map + x * (outerbound + 1) + (y + 1)));
        case INPUT_LEFT:
            return *((game_map + (x - 1) * (outerbound + 1) + y));
        case INPUT_RIGHT:
            return *((game_map + (x + 1) * (outerbound + 1) + y));
        default:
            return 0;
    }
}

void init_positions(player_t players[], int num_players, int outerbound, int *game_map)
{
    int x;
    int y;
    for(int i = 0; i < num_players; i++)
    {
        x = rand()%outerbound;
        y = rand()%outerbound;

        // Generate new coord until position is available 
        while(*((game_map + x * (outerbound + 1)) + y))
        {
            x = rand()%outerbound;
            y = rand()%outerbound;
        }
        *((game_map + x * (outerbound + 1)) + y) = players[i].id + 1;
        players[i].pos.x = x;
        players[i].pos.y = y;
    }
}

static void move_player(player_t * player, enum INPUTS input)
{
    switch(input)
    {
        case INPUT_UP:
            player->pos.y--;
        case INPUT_DOWN:
            player->pos.y++;
        case INPUT_LEFT:
            player->pos.x--;
        case INPUT_RIGHT:
            player->pos.x++;
        default:
            break;
    }
}

static void activate_skill(event_t *events, player_t *player)
{
    if(!(player->has_skill) || player->active_skill)
    {
        return;
    }
    player->has_skill--;
    player->active_skill = class_list[player->class_type].skill_duration;
    switch(player->class_type)
    {
        case(CLERIC):
            break;
        case(PALADIN):
            break;
        case(MAGE):
            break;
        case(WIZARD):
            break;
        default:
            break;
    }
}

int process_inputs(event_t *events, player_t players[], input_t inputs[], int outerbound, int *game_map)
{
    coord_t* pos;
    int slain = 0;
    for (int i = 0; i < MAX_PLAYERS; i++)
    {
        coord_t* pos;
        int collision;
        if(inputs[i].input == INPUT_SKILL)
        {
            // TODO activate skill prob needs active players and event
            continue;
        }
        pos = &players[i].pos;
        if(!check_inbounds(*pos, inputs[i].input, outerbound))
        {
            continue; // can't move
        }
        collision = check_collision(*pos, inputs[i].input, outerbound, game_map);
        if(!collision)
        {
            move_player(&players[i], inputs[i].input);
            continue;
        }
        slain += combat(events, &players[i], &players[collision]);
        players[i].active_skill = (players[i].active_skill - 1 < 0) ? 0 : players[i].active_skill--;
    }
    return slain;
}
