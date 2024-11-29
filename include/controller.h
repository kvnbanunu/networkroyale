#ifndef CONTROLLER_H
#define CONTROLLER_H

//int  check_controller(void);
//int  check_input(void);
//void read_controller_input(SDL_GameController *con);
void read_keyboard_input(void);

enum input
{
    IUP,
    IDOWN,
    ILEFT,
    IRIGHT,
    ISKILL
};

#endif    // CONTROLLER_H
