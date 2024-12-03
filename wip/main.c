#include "../include/input_handle.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_video.h>
#include <arpa/inet.h>
#include <ncurses.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define CON_UP 11
#define CON_DOWN 12
#define CON_LEFT 13
#define CON_RIGHT 14
#define KB_FIFO_PATH "kbfifo"
#define CON_FIFO_PATH "confifo"

int            check_controller(void);
_Noreturn void read_controller_input(SDL_GameController *con, int fd);


int main(void)
{
    int conflag;
    int fd_write_kb;
    int fd_write_con;
    int fd_read_kb;
    int fd_read_con;
    pid_t pid[2];

    // Create 2 FIFOs
    fifo_fd_write_setup(&fd_write_kb, KB_FIFO_PATH);
    fifo_fd_write_setup(&fd_write_con, CON_FIFO_PATH);

    // Must happen before any SDL related code
    if(SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("INIT ERR\n");
        return EXIT_FAILURE;
    }

    // check the controller
    conflag = check_controller();

    // fork the keyboard process
    pid[0] = fork();
    if (pid[0] == 0)
    {
        initscr();
        read_keyboard_input(fd_write_kb);
        close(fd_write_kb);
        endwin();
        return 0;
    }

    // fork the controller process if a controller was found
    if(conflag == 1)
    {
        pid[1] = fork();
        if (pid[1] == 0)
        {
            SDL_GameController *con = SDL_GameControllerOpen(0);
            read_controller_input(con, fd_write_con);
            return 0;
        }
    }

    // Open keyboard and controller fifos
    fifo_fd_read_setup(&fd_read_kb, KB_FIFO_PATH);
    fifo_fd_read_setup(&fd_read_con, CON_FIFO_PATH);


    int count = 0;
    while(count < 10)
    {
        int ret = poll_input(fd_read_kb, fd_read_con);
        printf("%d\n", ret);
        count++;
    }

    kill(pid[0], SIGKILL);
    endwin();
    kill(pid[1], SIGKILL);
    close(fd_read_con);
    close(fd_read_kb);
    unlink(KB_FIFO_PATH);
    unlink(CON_FIFO_PATH);
    SDL_Quit();

    return 0;
}

int check_controller(void)
{
    int con_count;

    con_count = SDL_NumJoysticks();

    if(con_count < 1)
    {
        // printf("No controller detected\n");
        return 0;
    }

    // printf("Controller detected, using controller input\n");
    return 1;
}

void read_controller_input(SDL_GameController *con, int fd)
{
    char      buf;
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
                int input = event.cbutton.button;
                switch(input)
                {
                    case 0:
                        buf = ISKILL;
                        write(fd, &buf, 1);
                        break;
                    case CON_UP:
                        buf = IUP;
                        write(fd, &buf, 1);
                        break;
                    case CON_DOWN:
                        buf = IDOWN;
                        write(fd, &buf, 1);
                        break;
                    case CON_LEFT:
                        buf = ILEFT;
                        write(fd, &buf, 1);
                        break;
                    case CON_RIGHT:
                        buf = IRIGHT;
                        write(fd, &buf, 1);
                        break;
                    default:
                        break;
                }
            }
        }
    }
}
