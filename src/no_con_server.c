#include "../include/no_con_game.h"
#include "../include/setup.h"
#include <netinet/in.h>
#include <stdlib.h>

int main(void)
{
    data_t  data = {0};
    char    host_address[INET_ADDRSTRLEN];
    uint8_t buf[AP_LEN];

    find_address(&data.host.sin_addr.s_addr, host_address);

    data.addr_len     = sizeof(struct sockaddr_in);
    data.fd           = setup_server(&data.host);
    data.seq_num      = 1;
    data.last_seq_num = 0;

    find_port(&data.host, host_address);
    // wait for other player
    recv(data.fd, buf, AP_LEN, 0);
    setup_client_unknown(&data.remote, buf);

    start(&data);
    data.running = 1;

    run(&data);
    cleanup(&data);
    return EXIT_SUCCESS;
}
