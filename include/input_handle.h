#ifndef INPUT_HANDLE_H
#define INPUT_HANDLE_H

void fifo_fd_write_setup(int *fd, const char *path);
void fifo_fd_read_setup(int *fd, const char *path);
void read_keyboard_input(int fd);
void poll_and_process_input(int kb_fd, int con_fd);

enum input
{
    IUP,
    IDOWN,
    ILEFT,
    IRIGHT,
    ISKILL
};

#endif    // INPUT_HANDLE_H
