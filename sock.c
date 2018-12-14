#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "sock.h"

int setup_server_socket(uint16_t port) {
    int server_fd;
    struct sockaddr_in server_addr;
    
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd < 0) {
        perror("socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(struct sockaddr_in));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons((uint16_t)port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in)) != 0) {
        perror("bind");
        goto clean_on_error;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen");
        goto clean_on_error;
    }

    return server_fd;

clean_on_error:
    close(server_fd);

    return -1;
}

int wait_client_conn(int server_fd) {
    int conn_fd;
    struct sockaddr_in client_addr;
    socklen_t socklen = sizeof(struct sockaddr_in);

    if ((conn_fd = accept(server_fd, (struct sockaddr*)&client_addr, &socklen)) < 0) {
        perror("accept");
        return -1;
    } else {
        printf("client addr: %s, port: %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    }

    return conn_fd;
}

int connect_server(char* server_name, uint16_t port) {
    int socket_fd;
    struct sockaddr_in server_addr;
    socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd < 0) {
        perror("socket");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_port = htons((uint16_t)port);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_name);
    if (connect(socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

int sock_sync_data(int conn_fd, char* send_buf, char* recv_buf, uint32_t size) {
    if (send(conn_fd, send_buf, size, 0) < 0) {
        perror("send");
        return -1;
    }

    int rc = recv(conn_fd, recv_buf, size, MSG_WAITALL);
    if (rc != size) {
        perror("recv");
        return -1;
    }

    return 0;
}