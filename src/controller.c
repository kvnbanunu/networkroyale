#include "../include/controller.h"
// #include <SDL2/SDL.h>
// #include <SDL2/SDL_error.h>
// #include <SDL2/SDL_events.h>
// #include <SDL2/SDL_gamecontroller.h>
// #include <SDL2/SDL_joystick.h>
// #include <SDL2/SDL_video.h>
#include <ncurses.h>
#include <stdio.h>

//int check_controller(void)
//{
//    SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
//
//    int con_count;
//    con_count = SDL_NumJoysticks();
//
//    if(con_count < 1)
//    {
//        printf("No controller detected\n");
//        SDL_Quit();
//        return 0;
//    }
//
//    printf("Controller detected, using controller input\n");
//    SDL_Quit();
//    return 1;
//}

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
                printw("space\n");
                refresh();
                break;
            case 'w':
                printw("w\n");
                break;
        }
        refresh();
    }
}

//void read_controller_input(SDL_GameController *con)
//{
//    SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);
//    SDL_Event event;
//
//    while(1)
//    {
//        while(SDL_PollEvent(&event))
//        {
//            if(event.type == SDL_QUIT)
//            {
//                SDL_GameControllerClose(con);
//                SDL_Quit();
//            }
//
//            if(event.type == SDL_CONTROLLERBUTTONDOWN)
//            {
//                // send udp enum
//                printf("Controller button %d\n", event.cbutton.button);
//            }
//        }
//    }
//}

// void controller(void)
// {
//     int                 con_count;
//     SDL_GameController *con;
//     SDL_Event           event;
//
//     con_count = SDL_NumJoysticks();
//     printf("Found: %d controllers\n", con_count);

// Determine connection
// if(con_count > 0)
// {
//     con = SDL_GameControllerOpen(0);
// }

// else
// {
//     con = NULL;
// }

// if(con == NULL)
// {
//     fprintf(stderr, "No controller opened");
// }

// while(1)
// {
//     while(SDL_PollEvent(&event))
//     {
//         if(event.type == SDL_QUIT)
//         {
//             SDL_GameControllerClose(con);
//             SDL_Quit();
//         }

//         else if(event.type == SDL_CONTROLLERBUTTONDOWN)
//         {
//             printf("controller: %d\n", event.cbutton.button);
//         }

//         else if(event.type == SDL_KEYDOWN)
//         {
//             printf("Key: %d\n", event.key.keysym.sym);
//         }
//     }
// }
