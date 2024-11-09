#include "../include/args.h"
#include "../include/socket.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#define UNKOWN_OPT_MESSAGE_LEN 48

void parse_args_s(int argc, char *argv[], char **addr, char **port, usage_func usage)
{
    int opt;

    opterr = 0;

    while((opt = getopt(argc, argv, "h")) != -1)
    {
        switch(opt)
        {
            case 'h':
            {
                usage(argv[0], EXIT_SUCCESS, NULL);
                break;
            }
            case '?':
            {
                char message[UNKOWN_OPT_MESSAGE_LEN];
                snprintf(message, sizeof(message), "Error: Unknown option '-%c'.", optopt);
                usage(argv[0], EXIT_FAILURE, message);
                break;
            }
            default:
            {
                usage(argv[0], EXIT_FAILURE, NULL);
            }
        }
    }

    if(optind >= argc)
    {
        usage(argv[0], EXIT_FAILURE, "Error: The ip address and port are required");
    }

    if(optind + 1 >= argc)
    {
        usage(argv[0], EXIT_FAILURE, "Error: The port is required");
    }

    if(optind < argc - 2)
    {
        usage(argv[0], EXIT_FAILURE, "Error: Too many arguments");
    }

    *addr = argv[optind];
    *port = argv[optind + 1];
}

void handle_args_s(const char *prog_name, const char *addr, const char *port_str, in_port_t *port, usage_func usage)
{
    if(addr == NULL)
    {
        usage(prog_name, EXIT_FAILURE, "Error: The ip address is required");
    }

    if(port_str == NULL)
    {
        usage(prog_name, EXIT_FAILURE, "Error: The port is required");
    }

    *port = parse_in_port_t(prog_name, port_str, usage);
}

// void parse_args_c(int argc, char *argv[])
//{
// }
//
// void handle_args_c(const char *prog_name)
//{
// }
