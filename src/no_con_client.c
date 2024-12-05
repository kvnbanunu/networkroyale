#include "../include/no_con_game.h"
#include "../include/setup.h"
#include <netinet/in.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    data_t data = {0};
    char   host_address[INET_ADDRSTRLEN];
    char  *remote_address;
    char  *port_str;

    find_address(&data.host.sin_addr.s_addr, host_address);

    data.addr_len     = sizeof(struct sockaddr_in);
    data.fd           = setup_server(&data.host);
    data.seq_num      = 1;
    data.last_seq_num = 0;

    parse_args(argc, argv, &remote_address, &port_str, &(data.port));
    setup_client_known(&data.remote, remote_address, data.port);
    send_client_info(data.fd, &data.host, &data.remote, data.addr_len);

    start(&data);
    data.running = 1;

    run(&data);
    cleanup(&data);
    return EXIT_SUCCESS;
}
