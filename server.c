#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "sock.h"
#include "ib.h"
#include "common.h"

void run_server(uint16_t port, uint8_t ib_port) {
    int server_fd;
    int conn_fd;
    ib_res res;
    qp_info local_qp_info;
    qp_info remote_qp_info;

    server_fd = setup_server_socket(port);
    if (server_fd < 0) {
        fprintf(stderr, "error to setup server socket\n");
        return;
    }
    printf("waiting for connection from client\n");

    conn_fd = wait_client_conn(server_fd);
    if (conn_fd < 0) {
        fprintf(stderr, "error to connect client\n");
        close(server_fd);
        return;
    }

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

    local_qp_info.lid = htons(res.port_attr.lid);
    local_qp_info.qpn = htonl(res.qp->qp_num);

    if (sock_sync_data(conn_fd, &local_qp_info, &remote_qp_info, sizeof(qp_info)) != 0) {
        fprintf(stderr, "error to sync qp info\n");
        goto clean;
    }

    printf("remote qp info: lid %d, qp_num %d\n", ntohs(remote_qp_info.lid), ntohl(remote_qp_info.qpn));

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
    close(conn_fd);
    close(server_fd);
}

int main(int argc, char **argv) {
    run_server(10010, atoi(argv[1]));

    exit(0);
}