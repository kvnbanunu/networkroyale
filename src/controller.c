#include "../include/controller.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_gamecontroller.h>
#include <SDL2/SDL_joystick.h>
#include <SDL2/SDL_video.h>
#include <stdio.h>

void controller(void)
{
    int                 con_count;
    SDL_GameController *con;
    SDL_Event           event;

    // init SDL
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s", SDL_GetError());
        return;
    }

    SDL_CreateWindow("Hidden Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1, 1, SDL_WINDOW_SHOWN);

    con_count = SDL_NumJoysticks();
    printf("Found: %d controllers\n", con_count);

    // Determine connection
    if(con_count > 0)
    {
        con = SDL_GameControllerOpen(0);
    }

    else
    {
        con = NULL;
    }

    if(con == NULL)
    {
        fprintf(stderr, "No controller opened");
    }

    while(1)
    {
        while(SDL_PollEvent(&event))
        {
            if(event.type == SDL_QUIT)
            {
                SDL_GameControllerClose(con);
                SDL_Quit();
            }

            else if(event.type == SDL_CONTROLLERBUTTONDOWN)
            {
                printf("controller: %d\n", event.cbutton.button);
            }

            else if(event.type == SDL_KEYDOWN)
            {
                printf("Key: %d\n", event.key.keysym.sym);
            }
        }
    }
}
