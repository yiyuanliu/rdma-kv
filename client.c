#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "sock.h"
#include "ib.h"
#include "common.h"

void run_client(char* addr, uint16_t port, uint8_t ib_port) {
    int sock_fd;
    ib_res res;
    qp_info local_qp_info;
    qp_info remote_qp_info;

    sock_fd = connect_server(addr, port);
    if (sock_fd < 0) {
        fprintf(stderr, "error to connect server\n");
        return;
    }
    printf("successful connected\n");

    res.send_buf_size = 1024;
    res.send_buf = malloc(1024);
    res.recv_buf_size = 1024;
    res.recv_buf = malloc(1024);
    if (create_ib_res(&res, ib_port)) {
        fprintf(stderr, "error to create ib_res\n");
        goto clean;
    }

    if (modify_qp_to_init(res.qp, ib_port)) {
        fprintf(stderr, "error to change qp to init\n");
        goto clean;
    }

    if (post_receive(&res)) {
        fprintf(stderr, "error to post recv\n");
        goto clean;
    }

    local_qp_info.lid = htons(res.port_attr.lid);
    local_qp_info.qpn = htonl(res.qp->qp_num);

    if (sock_sync_data(sock_fd, &local_qp_info, &remote_qp_info, sizeof(qp_info)) != 0) {
        fprintf(stderr, "error to sync qp info\n");
        goto clean;
    }

    printf("remote qp info: lid %ld, qp_num %ld\n", ntohs(remote_qp_info.lid), ntohl(remote_qp_info.qpn));

    if (modify_qp_to_rtr(res.qp, ib_port, ntohl(remote_qp_info.qpn), ntohs(remote_qp_info.lid))) {
        fprintf(stderr, "error to change qp to rtr\n");
        goto clean;
    }

    if (modify_qp_to_rts(res.qp)) {
        fprintf(stderr, "error to change qp to rts\n");
        goto clean;
    }

    printf("Successful to transfer qp state to rts\n");

clean:
    destroy_ib_res(&res);
    close(sock_fd);
}

int main(int argc, char **argv) {
    run_client(argv[1], 10010, atoi(argv[2]));

    exit(0);
}