#include "../include/setup.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BASE_TEN 10
#define MAX_ARGS 3    // [program, address, port]
#define PREFIX "192.168"

_Noreturn void usage(const char *prog_name, int exit_code, const char *message)
{
    if(message)
    {
        fprintf(stderr, "%s\n", message);
    }
    fprintf(stderr, "usage: %s [-h] [-i] <address> [-p] <port>\n", prog_name);
    fputs(" -h display this help message\n", stderr);
    fputs("If no arguments are passed to this program, this client will bind to a random free port.\n", stderr);
    fputs("The following are optional for the first player, but not for the second:\n", stderr);
    fputs(" -i <address> ip address of another player\n", stderr);
    fputs(" -p <port> port of another player\n", stderr);
    exit(exit_code);
}

static in_port_t parse_port(const char *prog_name, const char *port_str)
{
    char     *endptr;
    uintmax_t parsed_val;
    errno      = 0;
    parsed_val = strtoumax(port_str, &endptr, BASE_TEN);
    if(errno != 0)
    {
        perror("Error parsing port");
        exit(EXIT_FAILURE);
    }
    if(*endptr != '\0')
    {
        usage(prog_name, EXIT_FAILURE, "Invalid characters in port.");
    }
    if(parsed_val > UINT16_MAX)
    {
        usage(prog_name, EXIT_FAILURE, "Entered port is out of range.");
    }
    return (in_port_t)parsed_val;
}

void parse_args(int argc, char **argv, char **address, char **port_str, in_port_t *port)
{
    int opt;
    opterr = 0;

    while((opt = getopt(argc, argv, "h")) != -1)
    {
        switch(opt)
        {
            case 'h':
                usage(argv[0], EXIT_SUCCESS, NULL);
            case '?':
                usage(argv[0], EXIT_FAILURE, "Error: Unknown option");
            default:
                usage(argv[0], EXIT_FAILURE, NULL);
        }
    }
    if(optind >= argc)
    {
        usage(argv[0], EXIT_FAILURE, "Error: Unexpected arguments");
    }
    if(argc > MAX_ARGS)
    {
        usage(argv[0], EXIT_FAILURE, "Error: Too many arguments");
    }
    if(argc % 2 == 0)    // Should be odd
    {
        usage(argv[0], EXIT_FAILURE, "Error: Address and Port cannot be entered without one another");
    }

    *address  = argv[optind];
    *port_str = argv[optind + 1];

    if((*address == NULL || *port_str == NULL))
    {
        usage(argv[0], EXIT_FAILURE, "Error: Address and Port cannot be entered without one another");
    }
    *port = parse_port(argv[0], *port_str);
}

void find_address(in_addr_t *address, char *address_str)
{
    struct ifaddrs       *ifaddr;
    const struct ifaddrs *ifa;

    if(getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }
    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if(ifa->ifa_addr == NULL)
        {
            continue;
        }
        if(ifa->ifa_addr->sa_family == AF_INET)
        {
            if(getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), address_str, INET_ADDRSTRLEN, NULL, 0, NI_NUMERICHOST) != 0)
            {
                perror("getnameinfo");
                continue;
            }
            if(strncmp(address_str, PREFIX, strlen(PREFIX)) == 0)
            {
                inet_pton(AF_INET, address_str, address);
                break;
            }
        }
    }
    if(ifa == NULL)
    {
        freeifaddrs(ifaddr);
        perror("no address");
        exit(EXIT_FAILURE);
    }
    freeifaddrs(ifaddr);
}

int setup_server(struct sockaddr_in *addr)
{
    int       fd       = socket(AF_INET, SOCK_DGRAM, 0);    // NOLINT(android-cloexec-socket)
    socklen_t addr_len = sizeof(struct sockaddr);
    if(fd < 0)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    addr->sin_family = AF_INET;
    addr->sin_port   = 0;

    if(bind(fd, (struct sockaddr *)addr, sizeof(struct sockaddr_in)) < 0)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    // update addr to hold the address and port that the system detemined
    if(getsockname(fd, (struct sockaddr *)addr, &addr_len) == -1)
    {
        perror("getsockname");
        close(fd);
        exit(EXIT_FAILURE);
    }
    return fd;
}

// cppcheck-suppress constParameterPointer
void find_port(struct sockaddr_in *addr, const char host_address[INET_ADDRSTRLEN])
{
    char sbuf[NI_MAXSERV];
    if(getnameinfo((struct sockaddr *)addr, sizeof(struct sockaddr_in), NULL, 0, sbuf, sizeof(sbuf), NI_DGRAM | NI_NUMERICSERV) != 0)
    {
        perror("getnameinfo");
        exit(EXIT_FAILURE);
    }
    printf("Listening on %s, %s\n", host_address, sbuf);
}

void setup_client_known(struct sockaddr_in *addr, const char *addr_str, in_port_t port)
{
    if(inet_pton(AF_INET, addr_str, &addr->sin_addr) <= 0)
    {
        perror("Invalid address");
        exit(EXIT_FAILURE);
    }
    addr->sin_family = AF_INET;
    addr->sin_port   = htons(port);
}

// cppcheck-suppress constParameterPointer
void send_client_info(int fd, const struct sockaddr_in *local, struct sockaddr_in *remote, socklen_t addr_len)
{
    uint8_t buf[AP_LEN];
    memcpy(buf, &local->sin_addr, sizeof(in_addr_t));
    memcpy(buf + sizeof(in_addr_t), &local->sin_port, sizeof(in_port_t));
    sendto(fd, buf, AP_LEN, 0, (struct sockaddr *)remote, addr_len);
}

void setup_client_unknown(struct sockaddr_in *addr, const uint8_t buf[AP_LEN])
{
    addr->sin_family = AF_INET;
    memcpy(&addr->sin_addr.s_addr, buf, sizeof(in_addr_t));
    memcpy(&addr->sin_port, buf + sizeof(in_addr_t), sizeof(in_port_t));
}
