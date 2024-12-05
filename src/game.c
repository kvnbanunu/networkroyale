#include "../include/game.h"
#include <fcntl.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define PERCENT 100
// #define MAX_BUF 220

static const class_t class_list[] = {
    // ID | hp | atk | dmg | def | eva | cr | skill duration | skill_description
    {MOB,      5,  3,  2, 1, 0,  5,   0,  ""            },
    // Starting classes
    {CLERIC,   20, 6,  4, 4, 0,  5,   0,  "Full Heal"   },
    {FIGHTER,  15, 6,  3, 3, 0,  5,   5,  "Double Hit"  },
    {MAGE,     10, 6,  6, 1, 0,  5,   0,  "Teleport"    },
    {ROGUE,    10, 3,  4, 1, 25, 50,  5,  "Invisibility"},
    // Promoted classes
    {PALADIN,  30, 4,  0, 8, 0,  10,  0,  "Full Heal"   },
    {KNIGHT,   16, 5,  0, 3, 0,  10,  10, "Double Hit"  },
    {WIZARD,   10, 20, 0, 5, 0,  10,  0,  "Teleport"    },
    {ASSASSIN, 8,  3,  0, 2, 50, 100, 10, "Invisibility"}
};

// static const char skill_list[NUM_CLASSES][] = {"", "Full Heal", "Double Hit", "Teleport", "Invisibility", "Full Heal", "Double Hit", "Teleport", "Invisibility"};

void clear_stream(int stream)
{
    int copy = fcntl(stream, F_DUPFD_CLOEXEC);
    tcdrain(copy);
    tcflush(copy, TCIFLUSH);
    close(copy);
}

/* Converts name to only alphanumeric characters or a space */
static void check_name(char name[NAME_LEN])
{
    for (int i = 0; i < NAME_LEN - 1; i++)
    {
        if(
            (name[i] >= 'a' && name[i] <= 'z') ||
            (name[i] >= 'A' && name[i] <= 'Z') ||
            (name[i] >= '0' && name[i] <= '9')
        )
        {
            continue;
        }
        name[i] = ' ';
    }
    name[NAME_LEN] = '\0';
}

void join_game(uint8_t player_info[INFO_LEN], in_port_t *port)
{
    class_t     C        = class_list[CLERIC];
    class_t     F        = class_list[FIGHTER];
    class_t     M        = class_list[MAGE];
    class_t     R        = class_list[ROGUE];
    const char *name_msg = "Enter a username(max 7):\n";
    char     name[NAME_LEN];
    char     class_in = 0;
    int      class_home = 0;
    uint32_t class_net = 0;

    memset(name, '\0', NAME_LEN);
    write(STDOUT_FILENO, name_msg, strlen(name_msg));
    read(STDIN_FILENO, &name, NAME_LEN);
    check_name(name);
    clear_stream(STDIN_FILENO);
    printf("Hello %s\n", name);
    printf(
        "Select a starting class:\n\nClass:\t(1)Cleric\t(2)Fighter\t(3)Mage\t\t(4)Rogue\nHP:\t%d\t\t%d\t\t%d\t\t%d\nATK:\tD%d+%d\t\tD%d+%d\t\tD%d+%d\t\tD%d+%d\nDEF:\t%d\t\t%d\t\t%d\t\t%d\nEVA:\t%d\t\t%d\t\t%d\t\t%d\nCRIT:\t%d\t\t%d\t\t%d\t\t%d\nSkill:\t%s\t%s\t%s\t%s\n",
        C.hp,
        F.hp,
        M.hp,
        R.hp,
        C.atk,
        C.dmg_mod,
        F.atk,
        F.dmg_mod,
        M.atk,
        M.dmg_mod,
        R.atk,
        R.dmg_mod,
        C.def,
        F.def,
        M.def,
        R.def,
        C.eva,
        F.eva,
        M.eva,
        R.eva,
        C.crit_rate,
        F.crit_rate,
        M.crit_rate,
        R.crit_rate,
        C.skill_description,
        F.skill_description,
        M.skill_description,
        R.skill_description);
    clear_stream(STDOUT_FILENO);
    read(STDIN_FILENO, &class_in, 1);
    clear_stream(STDIN_FILENO);
    class_home = (class_in - '0');
    while(class_home < 1 || class_home > NUM_STARTING_CLASSES)
    {
        const char *invalid = "Invalid selection. Try again:\n";
        write(STDOUT_FILENO, invalid, strlen(invalid));
        read(STDIN_FILENO, &class_in, 1);
        clear_stream(STDIN_FILENO);
        class_home = (class_in - '0');
    }
    // Serialize player_info[]
    memcpy(player_info, port, sizeof(in_port_t)); // this port always stays in network byte order
    class_net = htonl((uint32_t)class_home);
    memcpy(&player_info[(int)(sizeof(in_port_t))], &class_net, sizeof(uint32_t));
    memcpy(&player_info[INFO_LEN - NAME_LEN], name, NAME_LEN);
}

static void add_event(event_t **event_head, uint16_t actor, uint16_t target, uint16_t dmg, uint16_t dodge, uint16_t death, uint16_t skill, uint16_t jobadv)
{
    event_t *curr  = *event_head;
    event_t *new_e = NULL;
    new_e          = (event_t *)malloc(sizeof(event_t));
    if(new_e == NULL)
    {
        perror("malloc add_event");
        exit(EXIT_FAILURE);
    }
    new_e->actor     = actor;
    new_e->target    = target;
    new_e->dmg       = dmg;
    new_e->dodge     = dodge;
    new_e->death     = death;
    new_e->skill_use = skill;
    new_e->jobadv    = jobadv;
    new_e->next      = NULL;

    while(curr->next != NULL)
    {
        curr = curr->next;
    }
    curr->next = new_e;
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
static int combat(event_t **event_head, player_t *p1, player_t *p2)
{
    player_t *a = p1;
    player_t *d = p2;
    int       dmg;
    uint16_t  dodge_flag = 0;
    int       num_slain  = 0;

    // determine combat turn order
    if(p1->class_type < p2->class_type)
    {
        a = p2;
        d = p1;
    }
    dmg        = dmg_calc(class_list[a->class_type], class_list[d->class_type], a->active_skill, d->active_skill);
    dodge_flag = (dmg == 0) ? 1 : 0;
    add_event(event_head, (uint16_t)(a->id), (uint16_t)(d->id), (uint16_t)dmg, dodge_flag, 0, 0, 0);
    if(d->hp - dmg <= 0)    // defender dies
    {
        add_event(event_head, (uint16_t)(d->id), (uint16_t)(a->id), 0, 0, 1, 0, 0);
        d->is_alive = 0;
        d->hp       = 0;
        a->exp++;
        num_slain = 1;
    }
    else
    {
        // Defenders turn to deal damage
        d->hp -= dmg;
        dmg        = dmg_calc(class_list[d->class_type], class_list[a->class_type], d->active_skill, a->active_skill);
        dodge_flag = (dmg == 0) ? 1 : 0;
        add_event(event_head, (uint16_t)(d->id), (uint16_t)(a->id), (uint16_t)dmg, dodge_flag, 0, 0, 0);
        if(a->hp - dmg <= 0)    // attacker dies
        {
            add_event(event_head, (uint16_t)(a->id), (uint16_t)(d->id), 0, 0, 1, 0, 0);
            a->is_alive = 0;
            a->hp       = 0;
            d->exp++;
            num_slain = 1;
        }
        a->hp -= dmg;
    }
    return num_slain;
}

static int check_inbounds(coord_t coord, int input)
{
    int inbound = 1;

    switch(input)
    {
        case INPUT_UP:
            inbound = (coord.y - 1 >= 0);
            break;
        case INPUT_DOWN:
            inbound = (coord.y + 1 < MAP_L);
            break;
        case INPUT_LEFT:
            inbound = (coord.x - 1 >= 0);
            break;
        case INPUT_RIGHT:
            inbound = (coord.x + 1 < MAP_W);
            break;
        default:
            break;
    }

    return inbound;
}

// If the (coord + input) on the map has a player on it, return the id(+1) or 0
static int check_collision(coord_t coord, int input, int game_map[MAP_W][MAP_L])
{
    int collision;
    int x = (int)coord.x;
    int y = (int)coord.y;

    switch(input)
    {
        case INPUT_UP:
            collision = game_map[x][y - 1];
            break;
        case INPUT_DOWN:
            collision = game_map[x][y + 1];
            break;
        case INPUT_LEFT:
            collision = game_map[x - 1][y];
            break;
        case INPUT_RIGHT:
            collision = game_map[x + 1][y];
            break;
        default:
            collision = 0;
    }

    return collision;
}

void init_positions(player_t players[], int num_players, int game_map[MAP_W][MAP_L])
{
    memset(game_map, 0, MAP_W * MAP_L * sizeof(int));
    for(int i = 0; i < num_players; i++)
    {
        int x = rand() % (MAP_W - 1);
        int y = rand() % (MAP_L - 1);

        // Generate new coord until position is available
        while(game_map[x][y] != 0)
        {
            x = rand() % (MAP_W - 1);
            y = rand() % (MAP_L - 1);
        }
        game_map[x][y]   = players[i].id + 1;
        players[i].pos.x = (uint16_t)x;
        players[i].pos.y = (uint16_t)y;
    }
}

static void move_player(player_t *player, int input, int game_map[MAP_W][MAP_L])
{
    if(input == INPUT_SKILL)
    {
        return;
    }

    // clear the original position on game_map
    game_map[player->pos.x][player->pos.y] = 0;

    switch(input)
    {
        case INPUT_UP:
            player->pos.y--;
            break;
        case INPUT_DOWN:
            player->pos.y++;
            break;
        case INPUT_LEFT:
            player->pos.x--;
            break;
        case INPUT_RIGHT:
            player->pos.x++;
            break;
        default:
            break;
    }

    // update position on the map
    game_map[player->pos.x][player->pos.y] = player->id + 1;
}

static void teleport(player_t *player, int game_map[MAP_W][MAP_L])
{
    int x                                  = rand() % (MAP_W - 1);
    int y                                  = rand() % (MAP_L - 1);
    game_map[player->pos.x][player->pos.y] = 0;
    while(game_map[x][y] != 0)
    {
        x = rand() % (MAP_W - 1);
        y = rand() % (MAP_L - 1);
    }
    player->pos.x  = (uint16_t)x;
    player->pos.y  = (uint16_t)y;
    game_map[x][y] = player->id + 1;
}

static void activate_skill(event_t **event_head, player_t *player, int game_map[MAP_W][MAP_L])
{
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
    add_event(event_head, (uint16_t)(player->id), 0, 0, 0, 0, 1, 0);
}

int process_inputs(event_t **event_head, player_t players[], input_t inputs[], int game_map[MAP_W][MAP_L])
{
    int slain = 0;
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        const coord_t *pos;
        int            collision;
        int in = (int)(inputs[i].input);
        if(in == INPUT_SKILL)
        {
            activate_skill(event_head, &players[i], game_map);
            continue;
        }
        pos = &players[i].pos;
        if(!check_inbounds(*pos, in))
        {
            continue;    // can't move
        }
        collision = check_collision(*pos, in, game_map);
        if(collision == 0)
        {
            move_player(&players[i], in, game_map);
            continue;
        }
        slain += combat(event_head, &players[i], &players[collision - 1]);
        if(players[i].active_skill > 0)
        {
            players[i].active_skill--;
        }
    }
    return slain;
}

static int serialize_event(uint8_t buf[], event_t **event, int dest)
{
    const event_t *e    = *event;
    int            flag = htons(e->actor + 1); // need to offset for easy checking
    memcpy(&buf[dest], &flag, 2);
    dest += 2;
    flag = htons(e->target + 1);
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

void serialize(uint8_t buf[], player_t players[], event_t **event_head)
{
    event_t *curr = (*event_head)->next;
    event_t *temp;
    int      dest = 0;
    memset(buf, 0, PACK_LEN);

    // copy the coordinates
    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        uint16_t x = htons(players[i].pos.x);
        uint16_t y = htons(players[i].pos.y);
        memcpy(buf + dest, &x, 2);
        dest += 2;
        memcpy(buf + dest, &y, 2);
        dest += 2;
    }

    // pack the events
    while(curr != NULL)
    {
        temp = curr;
        curr = curr->next;
        dest = serialize_event(buf, &temp, dest);
        free(temp);
    }

    (*event_head)->next = NULL;
}

const class_t *get_class_data(int class_type)
{
    const class_t *result = &(class_list[class_type]);
    return result;
}
