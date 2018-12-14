#ifndef __IB_H
#define __IB_H

#include <infiniband/verbs.h>

typedef struct ib_res {
    struct ibv_device_attr device_attr;
    struct ibv_port_attr port_attr;
    struct ibv_context *ib_ctx;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_qp *qp;
    struct ibv_mr *send_mr;
    struct ibv_mr *recv_mr;
    char *send_buf;
    size_t send_buf_size;
    char *recv_buf;
    size_t recv_buf_size;
} ib_res;

int create_ib_res(ib_res* res, uint8_t ib_port);
int modify_qp_to_init(struct ibv_qp *qp, uint8_t port);
int modify_qp_to_rtr(struct ibv_qp *qp, uint8_t ib_port, uint32_t dest_qpn, uint16_t dest_lid);
int modify_qp_to_rts(struct ibv_qp *qp);
int destroy_ib_res(struct ib_res *res);
int post_receive(struct ib_res *res);

#define CQ_SIZE         10
#define SEND_BUF_SIZE   1024
#define RECV_BUF_SIZE   1024

#define MAX_SEND_WR     10
#define MAX_RECV_WR     10

#define MAX_SEND_SGE    5
#define MAX_RECV_SGE    5

#endif