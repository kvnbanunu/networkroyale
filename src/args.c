#include "../include/args.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <inttypes.h>

#define UNKOWN_OPT_MESSAGE_LEN 48
#define BASE_TEN 10


_Noreturn void usage(const char *prog_name, int exit_code, const char *message)
{
    if(message)
    {
        fprintf(stderr, "%s\n", message);
    }

    // edit this later once the usage is defined
    fprintf(stderr, "usage: %s [-h] <address> <port>\n", prog_name);
    fputs("	-h  display this help message\n", stderr);
    exit(exit_code);
}

static in_port_t parse_in_port_t(const char *prog_name, const char *port_str)
{
    char     *endptr;
    uintmax_t parsed_val;

    errno      = 0;
    parsed_val = strtoumax(port_str, &endptr, BASE_TEN);

    if(errno != 0)
    {
        perror("Error parsing in_port_t");
        exit(EXIT_FAILURE);
    }

    // Check if there are any non-numeric characters in the input string
    if(*endptr != '\0')
    {
        usage(prog_name, EXIT_FAILURE, "Invalid characters in input.");
    }

    // Check if the parsed value is within the valid range for in_port_t
    if(parsed_val > UINT16_MAX)
    {
        usage(prog_name, EXIT_FAILURE, "in_port_t value out of range.");
    }
    return (in_port_t)parsed_val;
}

void parse_args(int argc, char *argv[], char **addr, char **port)
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

void handle_args(const char *prog_name, const char *addr, const char *port_str, in_port_t *port)
{
    if(addr == NULL)
    {
        usage(prog_name, EXIT_FAILURE, "Error: The ip address is required");
    }

    if(port_str == NULL)
    {
        usage(prog_name, EXIT_FAILURE, "Error: The port is required");
    }

    *port = parse_in_port_t(prog_name, port_str);
}
