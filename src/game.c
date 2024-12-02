#include "../include/game.h"
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PERCENT 100

static const class_t class_list[] = {
    // NPC     hp  atk dmg  def  eva   cr skill duration
    {MOB,      5,  3,  2, 1, 0,  5,   0 },
    // Starting classes
    {CLERIC,   20, 6,  4, 4, 0,  5,   0 },
    {FIGHTER,  15, 6,  3, 3, 0,  5,   5 },
    {MAGE,     10, 6,  6, 1, 0,  5,   0 },
    {ROGUE,    10, 3,  4, 1, 25, 50,  5 },
    // Promoted classes
    {PALADIN,  30, 4,  0, 8, 0,  10,  0 },
    {KNIGHT,   16, 5,  0, 3, 0,  10,  10},
    {WIZARD,   10, 20, 0, 5, 0,  10,  0 },
    {ASSASSIN, 8,  3,  0, 2, 50, 100, 10}
};

static void add_event(event_t *list, event_t *event)
{
    event_t curr = *list;

    // This would be the first event
    if(curr == NULL)
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

static void init_event(event_t *event, uint16_t actor, uint16_t target, uint16_t dmg, uint16_t dodge, uint16_t death, uint16_t skill, uint16_t jobadv)
{
    (*event)->actor     = actor;
    (*event)->target    = target;
    (*event)->dmg       = dmg;
    (*event)->dodge     = dodge;
    (*event)->death     = death;
    (*event)->skill_use = skill;
    (*event)->jobadv    = jobadv;
    (*event)->next      = NULL;
}

// Returns the damage c1 deals to c2, 0 == dodge
static int dmg_calc(class_t attacker, class_t defender, int a_skill, int d_skill)
{
    int result;
    int evasion_check;
    int crit_roll;
    int atk_dice = attacker.atk;
    int def_mod  = defender.def;

    if(d_skill && (defender.id == ROGUE || defender.id == ASSASSIN))
    {
        return 0;
    }

    evasion_check = rand() % PERCENT;
    if(evasion_check < defender.eva)
    {
        return 0;
    }

    crit_roll = rand() % PERCENT;
    if(crit_roll <= attacker.crit_rate)
    {
        def_mod = 0;
    }

    result = rand() % atk_dice;
    if(a_skill && (attacker.id == FIGHTER || attacker.id == KNIGHT))
    {
        result += rand() % atk_dice;
    }

    return result + attacker.dmg_mod - def_mod;
}

// Returns the amount of players slain this combat turn
static int combat(event_t *list, player_t *p1, player_t *p2)
{
    player_t *a = p1;
    player_t *d = p2;
    int       dmg;
    event_t   event;
    uint16_t  dodge_flag = 0;
    int       num_slain  = 0;

    // determine combat turn order
    if(p1->class_type < p2->class_type)
    {
        a = p2;
        d = p1;
    }
    dmg        = dmg_calc(class_list[a->class_type], class_list[d->class_type], a->active_skill, d->active_skill);
    event      = (event_t)malloc(sizeof(struct Event_Node));
    dodge_flag = (dmg == 0) ? 1 : 0;
    init_event(&event, (uint16_t)(a->id), (uint16_t)(d->id), (uint16_t)dmg, dodge_flag, 0, 0, 0);
    add_event(list, &event);
    if(d->hp - dmg <= 0)    // defender dies
    {
        event_t death = (event_t)malloc(sizeof(struct Event_Node));
        init_event(&death, (uint16_t)(d->id), (uint16_t)(a->id), 0, 0, 1, 0, 0);
        add_event(list, &death);
        d->is_alive = 0;
        d->hp       = 0;
        a->exp++;
        num_slain = 1;
    }
    else
    {
        // Defenders turn to deal damage
        event_t atk2 = (event_t)malloc(sizeof(struct Event_Node));
        d->hp -= dmg;
        dmg        = dmg_calc(class_list[d->class_type], class_list[a->class_type], d->active_skill, a->active_skill);
        dodge_flag = (dmg == 0) ? 1 : 0;
        init_event(&atk2, (uint16_t)(d->id), (uint16_t)(a->id), (uint16_t)dmg, dodge_flag, 0, 0, 0);
        add_event(list, &atk2);
        if(a->hp - dmg <= 0)    // attacker dies
        {
            event_t death = (event_t)malloc(sizeof(struct Event_Node));
            init_event(&death, (uint16_t)(a->id), (uint16_t)(d->id), 0, 0, 1, 0, 0);
            add_event(list, &death);
            a->is_alive = 0;
            a->hp       = 0;
            d->exp++;
            num_slain = 1;
        }
        a->hp -= dmg;
    }
    return num_slain;
}

static int check_inbounds(coord_t coord, enum INPUTS input)
{
    int inbound = 1;

    // Build system disallows enum + switch statement
    //    switch(input)
    //    {
    //        case INPUT_UP:
    //            inbound = (coord.y - 1 >= 0);
    //            break;
    //        case INPUT_DOWN:
    //            inbound = (coord.y + 1 <= MAP_BOUNDS);
    //            break;
    //        case INPUT_LEFT:
    //            inbound = (coord.x - 1 >= 0);
    //            break;
    //        case INPUT_RIGHT:
    //            inbound = (coord.x + 1 <= MAP_BOUNDS);
    //            break;
    //        default:
    //            inbound = 1;
    //    }

    if(input == INPUT_UP)
    {
        inbound = (coord.y - 1 >= 0);
    }
    if(input == INPUT_DOWN)
    {
        inbound = (coord.y + 1 <= MAP_BOUNDS);
    }
    if(input == INPUT_LEFT)
    {
        inbound = (coord.x - 1 >= 0);
    }
    if(input == INPUT_RIGHT)
    {
        inbound = (coord.x + 1 <= MAP_BOUNDS);
    }

    return inbound;
}

// If the (coord + input) on the map has a player on it, return the id(+1) or 0
static int check_collision(coord_t coord, enum INPUTS input, int game_map[MAP_BOUNDS + 1][MAP_BOUNDS + 1])
{
    int collision = 0;
    int x         = (int)coord.x;
    int y         = (int)coord.y;

    // Build system disallows enum + switch statement
    //    switch(input)
    //    {
    //        case INPUT_UP:
    //            collision = game_map[x][y - 1];
    //            break;
    //        case INPUT_DOWN:
    //            collision = game_map[x][y + 1];
    //            break;
    //        case INPUT_LEFT:
    //            collision = game_map[x - 1][y];
    //            break;
    //        case INPUT_RIGHT:
    //            collision = game_map[x + 1][y];
    //            break;
    //        case INPUT_SKILL:
    //            collision = 0;
    //            break;
    //        default:
    //            assert(0 && "Unhandled enum constant");
    //    }

    if(input == INPUT_UP)
    {
        collision = game_map[x][y - 1];
    }
    if(input == INPUT_DOWN)
    {
        collision = game_map[x][y + 1];
    }
    if(input == INPUT_LEFT)
    {
        collision = game_map[x - 1][y];
    }
    if(input == INPUT_RIGHT)
    {
        collision = game_map[x + 1][y];
    }
    return collision;
}

void init_positions(player_t players[], int num_players, int game_map[MAP_BOUNDS + 1][MAP_BOUNDS + 1])
{
    for(int i = 0; i < num_players; i++)
    {
        int x = rand() % MAP_BOUNDS;
        int y = rand() % MAP_BOUNDS;

        // Generate new coord until position is available
        while(game_map[x][y])
        {
            x = rand() % MAP_BOUNDS;
            y = rand() % MAP_BOUNDS;
        }
        game_map[x][y]   = players[i].id + 1;
        players[i].pos.x = (uint16_t)x;
        players[i].pos.y = (uint16_t)y;
    }
}

static void move_player(player_t *player, enum INPUTS input, int game_map[MAP_BOUNDS + 1][MAP_BOUNDS + 1])
{
    if(input == INPUT_SKILL)
    {
        return;
    }

    // clear the original position on game_map
    game_map[player->pos.x][player->pos.y] = 0;

    // Build system disallows enum with switch statement
    //    switch(input)
    //    {
    //        case INPUT_UP:
    //            player->pos.y--;
    //            break;
    //        case INPUT_DOWN:
    //            player->pos.y++;
    //            break;
    //        case INPUT_LEFT:
    //            player->pos.x--;
    //            break;
    //        case INPUT_RIGHT:
    //            player->pos.x++;
    //            break;
    //        case INPUT_SKILL:
    //            break;
    //        default:
    //            assert(0 && "Unhandled enum constant");
    //    }

    if(input == INPUT_UP)
    {
        player->pos.y--;
    }
    if(input == INPUT_DOWN)
    {
        player->pos.y++;
    }
    if(input == INPUT_LEFT)
    {
        player->pos.x--;
    }
    if(input == INPUT_RIGHT)
    {
        player->pos.x++;
    }

    // update position on the map
    game_map[player->pos.x][player->pos.y] = player->id + 1;
}

static void teleport(player_t *player, int game_map[MAP_BOUNDS + 1][MAP_BOUNDS + 1])
{
    int x                                  = rand() % MAP_BOUNDS;
    int y                                  = rand() % MAP_BOUNDS;
    game_map[player->pos.x][player->pos.y] = 0;
    while(game_map[x][y])
    {
        x = rand() % MAP_BOUNDS;
        y = rand() % MAP_BOUNDS;
    }
    player->pos.x  = (uint16_t)x;
    player->pos.y  = (uint16_t)y;
    game_map[x][y] = player->id + 1;
}

static void activate_skill(event_t *events, player_t *player, int game_map[MAP_BOUNDS + 1][MAP_BOUNDS + 1])
{
    event_t event;
    if(!(player->has_skill) || player->active_skill)
    {
        return;
    }
    player->has_skill--;
    player->active_skill = class_list[player->class_type].skill_duration;

    if(player->class_type == CLERIC || player->class_type == PALADIN)
    {
        player->hp = class_list[player->class_type].hp;    // full heal
    }
    else if(player->class_type == MAGE || player->class_type == WIZARD)
    {
        teleport(player, game_map);
    }

    event = (event_t)malloc(sizeof(struct Event_Node));
    if(event == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    init_event(&event, (uint16_t)(player->id), 0, 0, 0, 0, 1, 0);
    add_event(events, &event);
}

int process_inputs(event_t *events, player_t players[], input_t inputs[], int game_map[MAP_BOUNDS + 1][MAP_BOUNDS + 1])
{
    int slain = 0;
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        const coord_t *pos;
        int            collision;
        if(inputs[i].input == INPUT_SKILL)
        {
            activate_skill(events, &players[i], game_map);
            continue;
        }
        pos = &players[i].pos;
        if(!check_inbounds(*pos, inputs[i].input))
        {
            continue;    // can't move
        }
        collision = check_collision(*pos, inputs[i].input, game_map);
        if(!collision)
        {
            move_player(&players[i], inputs[i].input, game_map);
            continue;
        }
        slain += combat(events, &players[i], &players[collision - 1]);
        if(players[i].active_skill > 0)
        {
            players[i].active_skill--;
        }
    }
    return slain;
}

static int serialize_event(uint8_t buf[], event_t *event, int dest)
{
    event_t e    = *event;
    int     flag = htons(e->actor);
    memcpy(&buf[dest], &flag, 2);
    dest += 2;
    flag = htons(e->target);
    memcpy(&buf[dest], &flag, 2);
    dest += 2;
    flag = htons(e->dmg);
    memcpy(&buf[dest], &flag, 2);
    dest += 2;
    flag = htons(e->dodge);
    memcpy(&buf[dest], &flag, 2);
    dest += 2;
    flag = htons(e->death);
    memcpy(&buf[dest], &flag, 2);
    dest += 2;
    flag = htons(e->skill_use);
    memcpy(&buf[dest], &flag, 2);
    dest += 2;
    flag = htons(e->jobadv);
    memcpy(&buf[dest], &flag, 2);
    return dest + 2;
}

void serialize(uint8_t buf[], player_t players[], int player_count, event_t *events)
{
    event_t curr = *events;
    event_t temp;
    int     dest = 0;
    memset(buf, '\0', PACK_LEN);

    // copy the coordinates
    for(int i = 0; i < player_count; i++)
    {
        uint16_t x = htons(players[i].pos.x);
        uint16_t y = htons(players[i].pos.y);
        memcpy(&buf[dest], &x, 2);
        dest += 2;
        memcpy(&buf[dest], &y, 2);
        dest += 2;
    }

    // pack the events
    while(curr->next != NULL)
    {
        temp = curr;
        curr = curr->next;
        dest = serialize_event(buf, &temp, dest);
        free(temp);
    }
    // last one
    serialize_event(buf, &curr, dest);
    free(curr);
}
