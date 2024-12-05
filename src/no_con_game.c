#include "../include/no_con_game.h"
#include <arpa/inet.h>
#include <bits/pthreadtypes.h>
#include <bits/time.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static uint32_t calc_checksum(const pack_t *packet)
{
    return packet->x + packet->y + packet->seq_num;
}

static void pack(const pack_t *packet, uint8_t net_packet[PACK_LEN])
{
    uint32_t x        = htonl(packet->x);
    uint32_t y        = htonl(packet->y);
    uint32_t seq_num  = htonl(packet->seq_num);
    uint32_t checksum = htonl(packet->checksum);
    int      dest     = UINT32_SZ;
    memset(net_packet, 0, PACK_LEN);
    memcpy(net_packet, &x, UINT32_SZ);
    memcpy(net_packet + dest, &y, UINT32_SZ);
    dest += UINT32_SZ;
    memcpy(net_packet + dest, &seq_num, UINT32_SZ);
    dest += UINT32_SZ;
    memcpy(net_packet + dest, &checksum, UINT32_SZ);
}

static void send_coord(data_t *data)
{
    pack_t  packet = {data->l_pos.x, data->l_pos.y, data->seq_num, 0};
    uint8_t net_packet[PACK_LEN];
    packet.checksum = calc_checksum(&packet);
    pack(&packet, net_packet);

    sendto(data->fd, net_packet, PACK_LEN, 0, (struct sockaddr *)&data->remote, sizeof(data->remote));
    data->seq_num++;
}

static void init_position(coord_t *coord)
{
    coord->x = arc4random() % (MAP_W - 1);
    coord->y = arc4random() % (MAP_H - 1);
}

static void draw_border(void)
{
    clear();

    // Draw the border (the window has 1 extra row and column for the border)
    for(int x = 0; x <= MAP_W; x++)
    {
        mvprintw(0, x, "-");            // Top border
        mvprintw(MAP_H + 1, x, "-");    // Bottom border
    }
    for(int y = 0; y <= MAP_H; y++)
    {
        mvprintw(y, 0, "|");            // Left border
        mvprintw(y, MAP_W + 1, "|");    // Right border
    }

    // Draw the corners
    mvprintw(0, 0, "+");
    mvprintw(0, MAP_W + 1, "+");
    mvprintw(MAP_H + 1, 0, "+");
    mvprintw(MAP_H + 1, MAP_W + 1, "+");
}

static void add_delay(void)
{
    struct timespec req = {0};
    req.tv_sec          = 0;
    req.tv_nsec         = (long)(INPUT_DELAY * MILLISECOND);
    nanosleep(&req, NULL);
}

static void update_player_pos(data_t *data, int x, int y)
{
    pthread_mutex_lock(&data->mutex);
    if(x >= 0 && x < MAP_W && y >= 0 && y < MAP_H)
    {
        data->l_pos.x = (uint32_t)x;
        data->l_pos.y = (uint32_t)y;
        send_coord(data);
    }
    pthread_mutex_unlock(&data->mutex);
}

static void *keyboard_input(void *arg)
{
    data_t *data = (data_t *)arg;
    while(data->running)
    {
        int input = getch();
        if(input != ERR)
        {
            int x = (int)(data->l_pos.x);
            int y = (int)(data->l_pos.y);
            switch(input)
            {
                case KEY_UP:
                    y--;
                    break;
                case KEY_DOWN:
                    y++;
                    break;
                case KEY_LEFT:
                    x--;
                    break;
                case KEY_RIGHT:
                    x++;
                    break;
                case 'q':
                    data->running = 0;
                    break;
                default:
                    break;
            }
            data->timer = time(NULL);
            update_player_pos(data, x, y);
        }
        add_delay();
    }
    return NULL;
}

static void *random_movement(void *arg)
{
    data_t *data = (data_t *)arg;
    while(data->running)
    {
        if(difftime(time(NULL), data->timer) >= TIMEOUT_S)
        {
            int x = (int)(data->l_pos.x);
            int y = (int)(data->l_pos.x);
            switch(arc4random() % N_DIRECTIONS)
            {
                case INPUT_UP:
                    y--;
                    break;
                case INPUT_DOWN:
                    y++;
                    break;
                case INPUT_LEFT:
                    x--;
                    break;
                case INPUT_RIGHT:
                    x++;
                    break;
                default:
                    break;
            }
            data->timer = time(NULL);
            update_player_pos(data, x, y);
        }
        add_delay();
    }
    return NULL;
}

static void unpack(pack_t *packet, const uint8_t net_packet[PACK_LEN])
{
    uint32_t x;
    uint32_t y;
    uint32_t seq_num;
    uint32_t checksum;
    size_t   dest = UINT32_SZ;
    memcpy(&x, net_packet, UINT32_SZ);
    memcpy(&y, net_packet + dest, UINT32_SZ);
    dest += UINT32_SZ;
    memcpy(&seq_num, net_packet + dest, UINT32_SZ);
    dest += UINT32_SZ;
    memcpy(&checksum, net_packet + dest, UINT32_SZ);
    packet->x        = ntohl(x);
    packet->y        = ntohl(y);
    packet->seq_num  = ntohl(seq_num);
    packet->checksum = ntohl(checksum);
}

static void *receive_coord(void *arg)
{
    data_t *data   = (data_t *)arg;
    pack_t  packet = {0};
    uint8_t net_packet[PACK_LEN];
    while(data->running)
    {
        if(recvfrom(data->fd, net_packet, PACK_LEN, 0, (struct sockaddr *)&data->remote, &data->addr_len) > 0)
        {
            unpack(&packet, net_packet);
            if(packet.checksum == calc_checksum(&packet) && packet.seq_num > data->last_seq_num)
            {
                pthread_mutex_lock(&data->mutex);
                data->r_pos.x      = packet.x;
                data->r_pos.y      = packet.y;
                data->last_seq_num = packet.seq_num;
                pthread_mutex_unlock(&data->mutex);
            }
            add_delay();
        }
    }
    return NULL;
}

void start(data_t *data)
{
    // ncurses boilerplate
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    if(pthread_mutex_init(&data->mutex, NULL) != 0)
    {
        fprintf(stderr, "Error: Mutext initialization failed.\n");
        exit(EXIT_FAILURE);
    }
    data->timer = time(NULL);
    srand(data->port);    // NOLINT(cert-msc32-c,cert-msc51-cpp)
    init_position(&data->l_pos);
    send_coord(data);
}

void run(data_t *data)
{
    pthread_t keyboard_thread;
    pthread_t timeout_thread;
    pthread_t receive_thread;

    pthread_create(&keyboard_thread, NULL, keyboard_input, (void *)data);
    pthread_create(&timeout_thread, NULL, random_movement, (void *)data);
    pthread_create(&receive_thread, NULL, receive_coord, (void *)data);

    while(data->running)
    {
        pthread_mutex_lock(&data->mutex);
        draw_border();
        mvaddch((int)(data->l_pos.y + 1), (int)(data->l_pos.x + 1), '@');
        mvaddch((int)(data->r_pos.y + 1), (int)(data->r_pos.x + 1), '#');
        pthread_mutex_unlock(&data->mutex);
        refresh();
        add_delay();
    }

    pthread_join(keyboard_thread, NULL);
    pthread_join(timeout_thread, NULL);
    pthread_join(receive_thread, NULL);
}

void cleanup(const data_t *data)
{
    close(data->fd);
    endwin();
}
