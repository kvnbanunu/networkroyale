#include "../include/args.h"
#include "../include/game.h"
#include "../include/render.h"
#include "../include/setup.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_joystick.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

#define INPUT_PATH "input_fifo"
#define PERMISSION 0666
#define CON_UP 11
#define CON_DOWN 12
#define CON_LEFT 13
#define CON_RIGHT 14

struct Client_Data
{
    int id;
    int fd;
    int fifofd;
    int serverfd;
    in_addr_t address;
    in_port_t port;
    in_port_t server_tcp_port;
    in_port_t server_udp_port;
    struct sockaddr_in addr;
    struct sockaddr_in server_addr;
};

void send_input(struct Client_Data *data, int input, socklen_t addr_len);
void handle_response(struct Client_Data *data);
void     receive_initial_board(int sockfd, player_t players[MAX_PLAYERS]);
void fill_player_stats(player_t p[], int id);

int main(int argc, char *argv[])
{
    struct Client_Data c_data = {0};
    int                retval             = EXIT_FAILURE;
    char              *server_address_str = NULL;
    char              *server_port_str    = NULL;
    char               address_str[INET_ADDRSTRLEN];
    uint8_t            player_info[INFO_LEN];
    uint8_t buf[PACK_LEN];
    player_t           players[MAX_PLAYERS];
    WINDOW            *windows[N_WINDOWS];
    socklen_t addr_len = sizeof(struct sockaddr);
    pid_t pids[2];
    int npids = 0;

    parse_args(argc, argv, &server_address_str, &server_port_str);
    handle_args(argv[0], server_address_str, server_port_str, &(c_data.server_tcp_port));

    findaddress(&(c_data.address), address_str);
    c_data.port = setup_and_bind(&(c_data.fd), &(c_data.addr), c_data.address, addr_len, SOCK_DGRAM, 0);

    join_game(player_info, &(c_data.port));

    setup_and_connect(&(c_data.serverfd), &(c_data.server_addr), server_address_str, c_data.server_tcp_port, addr_len);

    write(c_data.serverfd, player_info, INFO_LEN);

    handle_response(&c_data);

    receive_initial_board(c_data.serverfd, players);

    fill_player_stats(players, c_data.id);

    socket_close(c_data.serverfd);
    c_data.server_addr.sin_port = c_data.server_udp_port;

    //------------------------------INITIAL RENDER-----------------------------------------------
    r_setup(windows);
    r_init(players, windows, c_data.id);
    //------------------------------GAME STARTS HERE-----------------------------------------------

    if(mkfifo(INPUT_PATH, PERMISSION) == -1)
    {
        perror("mkfifo");
        socket_close(c_data.fd);
        exit(EXIT_FAILURE);
    }

    c_data.fifofd = open(INPUT_PATH, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if(c_data.fifofd < 0)
    {
        perror("Failed to open fifo");
        socket_close(c_data.fd);
        unlink(INPUT_PATH);
        exit(EXIT_FAILURE);
    }

    if(SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0)
    {
        perror("sdl init");
        socket_close(c_data.fd);
        close(c_data.fifofd);
        unlink(INPUT_PATH);
        exit(EXIT_FAILURE);
    }

    pids[0] = fork();
    if(pids[0] < 0)
    {
        perror("fork");
        socket_close(c_data.fd);
        close(c_data.fifofd);
        unlink(INPUT_PATH);
        exit(EXIT_FAILURE);
    }
    if(pids[0] == 0)
    {
        socket_close(c_data.fd);
        int fd = open(INPUT_PATH, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
        if(fd < 0)
        {
            return EXIT_FAILURE;
        }
        while(1)
        {
            int input = getch();
            int output = 0;
            switch(input)
            {
                case KEY_UP:
                    output = INPUT_UP;
                    break;
                case KEY_DOWN:
                    output = INPUT_DOWN;
                    break;
                case KEY_LEFT:
                    output = INPUT_LEFT;
                    break;
                case KEY_RIGHT:
                    output = INPUT_RIGHT;
                    break;
                case ' ':
                    output = INPUT_SKILL;
                    break;
                default:
                    continue;
            }
            write(fd, &output, sizeof(int));
        }
        close(fd);
    }
    npids++;
    
    if(SDL_NumJoysticks() > 0)
    {
        npids++;
        pids[1] = fork();
        if(pids[1] < 0)
        {
            perror("fork");
            socket_close(c_data.fd);
            close(c_data.fifofd);
            unlink(INPUT_PATH);
            exit(EXIT_FAILURE);
        }
        if(pids[1] == 0)
        {
            int fd;
            SDL_Event event;
            socket_close(c_data.fd);
            fd = open(INPUT_PATH, O_WRONLY | O_NONBLOCK | O_CLOEXEC);
            if(fd < 0)
            {
                return EXIT_FAILURE;
            }
            SDL_GameController *con = SDL_GameControllerOpen(0);
            while(1)
            {
                while(SDL_PollEvent(&event))
                {
                    if(event.type == SDL_QUIT)
                    {
                        SDL_GameControllerClose(con);
                        SDL_Quit();
                    }
                    if(event.type == SDL_CONTROLLERBUTTONDOWN)
                    {
                        int input = event.cbutton.button;
                        int output;
                        switch(input)
                        {
                            case 0:
                                output = INPUT_SKILL;
                                break;
                            case CON_UP:
                                output = INPUT_UP;
                                break;
                            case CON_DOWN:
                                output = INPUT_DOWN;
                                break;
                            case CON_LEFT:
                                output = INPUT_LEFT;
                                break;
                            case CON_RIGHT:
                                output = INPUT_RIGHT;
                                break;
                            default:
                                continue;
                        }
                        write(fd, &output, sizeof(int));
                    }
                }
            }
        }
    }
    SDL_Quit();
    //------------------------------GAME LOOP-----------------------------------------------

    while(1) // replace with signal
    {
        // get inputs
        int input;
        clear_stream(c_data.fifofd);
        read(c_data.fifofd, &input, sizeof(int));
        send_input(&c_data, input, addr_len);
        recv(c_data.fd, buf, PACK_LEN, 0);
        r_update(players, windows, c_data.id, buf);
    }

    for(int i = 0; i < N_WINDOWS; i++)
    {
        delwin(windows[i]);
    }
    curs_set(1);
    endwin();
    //-----------------------------------------------------------------------------
    retval = EXIT_SUCCESS;
    socket_close(c_data.fd);
    for (int i = 0; i < npids; i++)
    {
        kill(pids[i], SIGTERM);
    }
    return retval;
}

void send_input(struct Client_Data *data, int input, socklen_t addr_len)
{
    uint8_t buf[INPUT_SIZE];
    uint16_t id = htons(data->id);
    uint32_t in = htonl(input);
    memcpy(buf, &id, sizeof(uint16_t));
    memcpy(buf + sizeof(uint16_t), &in, sizeof(uint32_t));
    sendto(data->fd, buf, INPUT_SIZE, 0, (struct sockaddr *)&(data->server_addr), addr_len);
}

void handle_response(struct Client_Data *data)
{
    uint8_t  response[sizeof(in_port_t) + sizeof(uint32_t)];
    uint32_t playerid;
    read(data->serverfd, response, sizeof(response));
    memcpy(&(data->server_udp_port), response, sizeof(in_port_t));
    memcpy(&playerid, response + sizeof(in_port_t), sizeof(uint32_t));
    data->id = (int)(ntohl(playerid));

    printf("player id: %u\n", data->id);
}

void receive_initial_board(int sockfd, player_t players[MAX_PLAYERS])
{
    uint8_t buf[INIT_BOARD_BUF_LEN];
    int     dest = 0;

    memset(players, 0, sizeof(player_t) * MAX_PLAYERS);

    read(sockfd, buf, INIT_BOARD_BUF_LEN);

    for(int i = 0; i < MAX_PLAYERS; i++)
    {
        player_t *p = &(players[i]);
        uint32_t  net_class;
        uint16_t  net_x;
        uint16_t  net_y;
        p->id       = i;
        p->is_alive = 1;
        memcpy(&net_class, &buf[dest], sizeof(uint32_t));
        dest += (int)(sizeof(uint32_t));
        memcpy(&net_x, &buf[dest], sizeof(uint16_t));
        dest += (int)(sizeof(uint16_t));
        memcpy(&net_y, &buf[dest], sizeof(uint16_t));
        dest += (int)(sizeof(uint16_t));
        p->class_type = (int)(ntohl(net_class));
        p->pos.x      = ntohs(net_x);
        p->pos.y      = ntohs(net_y);
        memcpy(p->username, &buf[dest], NAME_LEN);
        dest += NAME_LEN;
    }
}

void fill_player_stats(player_t p[], int id)
{
    const class_t *class = get_class_data(p[id].class_type);
    p[id].hp = class->hp;
    p[id].has_skill = 1;
}
