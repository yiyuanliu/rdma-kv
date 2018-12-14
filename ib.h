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
    struct ibv_mr *mr;
    char *buf;
} ib_res;

int create_ib_res(ib_res* res, uint8_t ib_port);


#endif