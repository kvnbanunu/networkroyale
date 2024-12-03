#include <stdio.h>
#include "../include/controller.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <poll.h>
#include <errno.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_gamecontroller.h>

void read_controller_input(SDL_GameController *con, int fd);
int check_controller(void);

int main() {
    int conflag;

    int fd_write_kb;
    int fd_write_con;

    int fd_read_kb;
    int fd_read_con;

    printf("Starting main program...\n");

    
    // Create 2 FIFOs
    fifo_fd_write_setup(&fd_write_kb, "kbfifo");
    fifo_fd_write_setup(&fd_write_con, "confifo");

    // init sdl subsys
    if(SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("INIT ERR\n");
        return EXIT_FAILURE;
    }

    // check the controller
    conflag = check_controller();

    // fork the keyboard process
    if (fork() == 0) {
        read_keyboard_input(fd_write_kb);
        close(fd_write_kb);
        return EXIT_FAILURE;
    }

    // fork the controller process if a controller was found
    if (conflag == 1 && fork() == 0)
    {
        SDL_GameController *con = SDL_GameControllerOpen(0);
        read_controller_input(con, fd_write_con);
        close(fd_write_con);
        return EXIT_FAILURE;
    }

    // Open keyboard and controller fifos
    fifo_fd_read_setup(&fd_read_kb, "kbfifo");
    fifo_fd_read_setup(&fd_read_con, "confifo");

    

    poll_and_process_input(fd_read_kb, fd_read_con);

    close(fd_read_con);
    close(fd_read_kb);
    unlink("confifo");
    unlink("kbfifo");

    return EXIT_FAILURE;
}

void read_controller_input(SDL_GameController *con, int fd)
{
    char      buf;
    int input;
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
                input = event.cbutton.button;
                
                switch(input)
                {
                    case 0:
                        buf = ISKILL;
                        write(fd, &buf, 1);
                        break;
                    case 11:
                        buf = IUP;
                        write(fd, &buf, 1);
                        break;
                    case 12:
                        buf = IDOWN;
                        write(fd, &buf, 1);
                        break;
                    case 13:
                        buf = ILEFT;
                        write(fd, &buf, 1);
                        break;
                    case 14:
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