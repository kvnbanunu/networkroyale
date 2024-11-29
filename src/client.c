#include "../include/args.h"
// #include "../include/controller.h"
#include "../include/setup.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_video.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ncurses.h>
#define NAME_LEN 5
#define INFO_LEN 7
#define GAME_START_LEN 17

int            check_controller(void);
_Noreturn void read_controller_input(SDL_GameController *con);
_Noreturn void read_keyboard_input(void);
int main(int argc, char *argv[])
{
    int                serverfd;
    int                udpfd;
    int                retval             = EXIT_FAILURE;
    char              *server_address_str = NULL;
    char              *server_port_str    = NULL;
    char               address_str[INET_ADDRSTRLEN];
    const char        *message;
    char               username[NAME_LEN];
    uint8_t            player_info[INFO_LEN];
    char               game_start_message[GAME_START_LEN + 1];
    in_addr_t          address;
    in_port_t          server_port;
    in_port_t          udp_port;
    in_port_t          server_udp_port;
    struct sockaddr_in server_addr;
    struct sockaddr_in udp_addr;
    socklen_t          addr_len = sizeof(struct sockaddr);

    int conflag;

    parse_args(argc, argv, &server_address_str, &server_port_str);
    handle_args(argv[0], server_address_str, server_port_str, &server_port);

    message = "Enter a username:\n";
    write(1, message, strlen(message) + 1);
    read(1, username, NAME_LEN);
    findaddress(&address, address_str);
    udp_port = setup_and_bind(&udpfd, &udp_addr, address, addr_len, SOCK_DGRAM);

    // serializing player info [name, address, port]
    memset(player_info, '\0', INFO_LEN);
    memcpy(player_info, username, NAME_LEN);
    memcpy(&player_info[NAME_LEN], &udp_port, sizeof(in_port_t));

    setup_and_connect(&serverfd, &server_addr, server_address_str, server_port, addr_len);

    write(serverfd, player_info, INFO_LEN);

    read(serverfd, &server_udp_port, sizeof(in_port_t));

    memset(game_start_message, '\0', GAME_START_LEN + 1);
    read(serverfd, game_start_message, GAME_START_LEN);

    printf("%s", game_start_message);
    socket_close(serverfd);

    // Must happen before any SDL related code
    if(SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("INIT ERR\n");
        return EXIT_FAILURE;
    }

    conflag = check_controller();

    if(conflag == 1)
    {
        SDL_GameController *con = SDL_GameControllerOpen(0);
        read_controller_input(con);
    }

    // Start the game here
    retval = EXIT_SUCCESS;

    /*
     * while(1)
     * get_input <--
     *
     *
     *
     * send_input
     * read_input <-- blocks until server updates
     * render
     */

    SDL_Quit();
    socket_close(udpfd);
    return retval;
}

int check_controller(void)
{
    int con_count;

    con_count = SDL_NumJoysticks();

    if(con_count < 1)
    {
        printf("No controller detected\n");
        return 0;
    }

    printf("Controller detected, using controller input\n");
    return 1;
}

void read_controller_input(SDL_GameController *con)
{
    SDL_Event event;
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
                // send udp enum
                printf("Controller button %d\n", event.cbutton.button);
            }
        }

        switch(getch())
        {
            case KEY_RIGHT:
                printw("right\n");
                refresh();
                //send udp
                break;
            case KEY_LEFT:
                printw("left\n");
                refresh();
                //send udp
                break;
            case KEY_UP:
                printw("up\n");
                refresh();
                //send udp
                break;
            case KEY_DOWN:
                printw("down\n");
                refresh();
                //send udp
                break;
            case ' ':
                break;
            case 'w':
                break;
            case 'a':
                break;
            case 's':
                break;
            case 'd':
                break;

        }
    }
}


void read_keyboard_input(void)
{
    initscr();
    keypad(stdscr, TRUE);
    noecho();
    cbreak();
    while(1)
    {
    
        switch(getch())
        {
            case KEY_RIGHT:
                printw("right\n");
                refresh();
                //send udp
                break;
            case KEY_LEFT:
                printw("left\n");
                refresh();
                //send udp
                break;
            case KEY_UP:
                printw("up\n");
                refresh();
                //send udp
                break;
            case KEY_DOWN:
                printw("down\n");
                refresh();
                //send udp
                break;
            case ' ':
                break;
            case 'w':
                break;
            case 'a':
                break;
            case 's':
                break;
            case 'd':
                break;
        }
        refresh();
    }
}

