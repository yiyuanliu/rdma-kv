#ifndef __COMMON_H
#define __COMMON_H

#include <stdint.h>

typedef struct qp_info {
    uint32_t qpn;
    uint16_t lid;
} qp_info __attribute__((packed));

#endif