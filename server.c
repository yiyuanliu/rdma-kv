#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sock.h"

void run_server(uint16_t port) {
    int server_fd = setup_server_socket(port);
    if (server_fd < 0) {
        fprintf(stderr, "error to setup server socket\n");
        return;
    }

    int conn_fd = wait_client_conn(server_fd);
    if (conn_fd < 0) {
        fprintf(stderr, "error to connect client\n");
        close(server_fd);
        return;
    }

    char buf1[20] = "1234567890987654321\0";
    char buf2[20];
    if (sock_sync_data(conn_fd, buf1, buf2, 20) != 0) {
        fprintf(stderr, "error to sync data\n");
        close(conn_fd);
        close(server_fd);
        return;
    }

    printf("%s\n", buf2);
    close(conn_fd);
    close(server_fd);
}

int main(int argc, char **argv) {
    run_server(10010);

    exit(0);
}