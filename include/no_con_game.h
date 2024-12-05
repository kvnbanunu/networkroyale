#ifndef GAME_H
#define GAME_H

#include <signal.h>
#include <time.h>
#include <netinet/in.h>
#include <stdint.h>
#include <pthread.h>

#define PACK_LEN 16
#define TIMEOUT_S 5
#define N_DIRECTIONS 3 //off by 1
#define UINT32_SZ 4
#define INPUT_DELAY 50
#define MILLISECOND 1000000
#define MAP_W 32
#define MAP_H 16

typedef struct Coord
{
    uint32_t x;
    uint32_t y;
} coord_t;

enum INPUTS
{
    INPUT_UP,
    INPUT_DOWN,
    INPUT_LEFT,
    INPUT_RIGHT
};

typedef struct Packet
{
    uint32_t x;
    uint32_t y;
    uint32_t seq_num;
    uint32_t checksum;
} pack_t;

typedef struct Data
{
    struct sockaddr_in host;
    struct sockaddr_in remote;
    int fd;
    volatile sig_atomic_t running;
    socklen_t addr_len;
    in_port_t port; // home byte order
    uint32_t seq_num;
    uint32_t last_seq_num;
    coord_t l_pos;
    coord_t r_pos;
    time_t timer;
    pthread_mutex_t mutex;
} data_t;

void start(data_t *data);
void run(data_t *data);
void cleanup(const data_t *data);

#endif // GAME_H
