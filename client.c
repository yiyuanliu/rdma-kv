#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sock.h"

void run_client(char* addr, uint16_t port) {
    int sock_fd = connect_server(addr, port);
    if (sock_fd < 0) {
        fprintf(stderr, "error to connect server\n");
        return;
    }

    char buf1[20] = "9876543210123456789\0";
    char buf2[20];
    if (sock_sync_data(sock_fd, buf1, buf2, 20) != 0) {
        fprintf(stderr, "error to sync data\n");
        close(sock_fd);
        return;
    }

    printf("%s\n", buf2);
    close(sock_fd);
}

int main(int argc, char **argv) {
    run_client(argv[1], 10010);

    exit(0);
}