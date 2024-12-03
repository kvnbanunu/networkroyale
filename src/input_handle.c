#include "../include/input_handle.h"
#include <errno.h>
#include <fcntl.h>
#include <ncurses.h>
#include <poll.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PERMISSION 0666

void fifo_fd_write_setup(int *fd, const char *path)
{
    if(mkfifo(path, PERMISSION) == -1 && errno != EEXIST)
    {
        perror("mkfifo");
        return;
    }

    *fd = open(path, O_RDWR | O_NONBLOCK | O_CLOEXEC);
    if(*fd < 0)
    {
        perror("Failed to open FIFO for writing");
        return;
    }
}

void fifo_fd_read_setup(int *fd, const char *path)
{
    *fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if(*fd < 0)
    {
        perror("Failed to open keyboard FIFO for reading");
        return;
    }
}

// NOTE: THE KEYBOARD INPUTS WILL ONLY BE READ IF initscr() WAS CALLED
_Noreturn void read_keyboard_input(int fd)
{
    char buf;

    // this initscreen() was for debugging
    // initscr();
    keypad(stdscr, TRUE);
    noecho();
    cbreak();

    while(1)
    {
        switch(getch())
        {
            case KEY_RIGHT:
                buf = IRIGHT;
                write(fd, &buf, 1);
                break;
            case KEY_LEFT:
                buf = ILEFT;
                write(fd, &buf, 1);
                break;
            case KEY_UP:
                buf = IUP;
                write(fd, &buf, 1);
                break;
            case KEY_DOWN:
                buf = IDOWN;
                write(fd, &buf, 1);
                break;
            case ' ':
                buf = ISKILL;
                write(fd, &buf, 1);
                refresh();    // refresh?
                break;
            case 'w':
                buf = IUP;
                write(fd, &buf, 1);
                refresh();
                break;
            case 'a':
                buf = ILEFT;
                write(fd, &buf, 1);
                break;
            case 's':
                buf = IDOWN;
                write(fd, &buf, 1);
                break;
            case 'd':
                buf = IRIGHT;
                write(fd, &buf, 1);
                break;
            default:
                break;
        }
        refresh();
    }
}

int poll_input(int kbfd, int confd)
{
    int           result;
    struct pollfd pfd[2];
    pfd[0].fd     = kbfd;
    pfd[0].events = POLLIN;

    pfd[1].fd     = confd;
    pfd[1].events = POLLIN;

    while(1)
    {
        int ret = poll(pfd, 2, -1);
        if(ret == -1)
        {
            perror("poll");
            exit(EXIT_FAILURE);
        }
        if(ret > 0)
        {
            char buf;
            if(pfd[0].revents & POLLIN)
            {
                if(read(kbfd, &buf, 1) < 1)
                {
                    continue;
                }
            }
            if(pfd[1].revents & POLLIN)
            {
                if(read(confd, &buf, 1) < 1)
                {
                    continue;
                }
            }
            result = (int)buf;
            if(result >= ILEFT || result <= ISKILL)
            {
                break;
            }
            continue;
        }
    }
    return result;
}

// Need to pass in a file descriptor to the udp, and send the correct inputs over the server
void poll_and_process_input(int kb_fd, int con_fd)
{
    struct pollfd pfd[2];
    pfd[0].fd     = kb_fd;
    pfd[0].events = POLLIN;

    pfd[1].fd     = con_fd;
    pfd[1].events = POLLIN;

    while(1)
    {
        int ret = poll(pfd, 2, -1);
        if(ret == -1)
        {
            perror("poll");
            break;
        }

        if(ret > 0)
        {
            if(pfd[0].revents & POLLIN)
            {
                char    buffer[2];
                ssize_t bytes_read = read(kb_fd, buffer, sizeof(buffer) - 1);
                if(bytes_read > 0)
                {
                    buffer[bytes_read] = '\0';
                    printf("Data from FIFO 1: %s\n", buffer);
                    if(buffer[0] == (char)IRIGHT)
                    {
                        printf("Command from FIFO 1: Move Right\n");
                    }
                    else if(buffer[0] == (char)ILEFT)
                    {
                        printf("Command from FIFO 1: Move Left\n");
                    }
                    else if(buffer[0] == (char)IUP)
                    {
                        printf("Command from FIFO 1: Move Up\n");
                    }
                    else if(buffer[0] == (char)IDOWN)
                    {
                        printf("Command from FIFO 1: Move Down\n");
                    }
                    else if(buffer[0] == (char)ISKILL)
                    {
                        printf("Command from FIFO 1: Use Skill\n");
                    }
                    else
                    {
                        printf("Unknown Command from FIFO 1\n");
                    }
                }
                else if(bytes_read == 0)
                {
                    printf("Writer disconnected on FIFO 1.\n");
                }
                else
                {
                    perror("Read error on FIFO 1");
                }
            }

            if(pfd[1].revents & POLLIN)
            {
                char    buffer[2];
                ssize_t bytes_read = read(con_fd, buffer, sizeof(buffer) - 1);
                if(bytes_read > 0)
                {
                    buffer[bytes_read] = '\0';
                    printf("Data from FIFO 2: %s\n", buffer);
                    if(buffer[0] == (char)IRIGHT)
                    {
                        printf("Command from FIFO 2: Move Right\n");
                    }
                    else if(buffer[0] == (char)ILEFT)
                    {
                        printf("Command from FIFO 2: Move Left\n");
                    }
                    else if(buffer[0] == (char)IUP)
                    {
                        printf("Command from FIFO 2: Move Up\n");
                    }
                    else if(buffer[0] == (char)IDOWN)
                    {
                        printf("Command from FIFO 2: Move Down\n");
                    }
                    else if(buffer[0] == (char)ISKILL)
                    {
                        printf("Command from FIFO 2: Use Skill\n");
                    }
                    else
                    {
                        printf("Unknown Command from FIFO 2\n");
                    }
                }
                else if(bytes_read == 0)
                {
                    printf("Writer disconnected on FIFO 2.\n");
                }
                else
                {
                    perror("Read error on FIFO 2");
                }
            }
        }
    }
}
