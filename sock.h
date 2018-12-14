#ifndef __SOCK_H
#define __SOCK_H

#include <stdint.h>

int setup_server_socket(uint16_t port);
int wait_client_conn(int server_fd);
int connect_server(char* server_name, uint16_t port);
int sock_sync_data(int conn_fd, char* send_buf, char* recv_buf, uint32_t size);

#endif