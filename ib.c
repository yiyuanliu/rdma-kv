#include "ib.h"
#include <stdio.h>

int create_ib_res(ib_res* res, uint8_t ib_port) {
    struct ibv_device **dev_list = NULL;
    struct ibv_qp_init_attr qp_init_attr;
    struct ibv_device *ib_dev = NULL;
    size_t size;
    int i;
    int mr_flags = 0;
    int cq_size = 0;
    int num_devices;
    char* dev_name;

    dev_list = ibv_get_device_list(&num_devices);
	if (!dev_list) {
		fprintf(stderr, "failed to get IB devices list\n");
		return 1;
	}

	/* if there isn't any IB device in host */
	if (!num_devices) {
		fprintf(stderr, "found %d device(s)\n", num_devices);
		return 1;
	}

    fprintf(stdout, "found %d device(s)\n", num_devices);
    dev_name = strdup(ibv_get_device_name(dev_list[1])); 
	fprintf(stdout, "using first one found: %s\n", dev_name);
    res->ib_ctx = ibv_open_device(ib_dev);
	if (!res->ib_ctx) {
		fprintf(stderr, "failed to open device %s\n", dev_name);
        free(dev_name);
		return 1;
	}
    free(dev_name);

    /* We are now done with device list, free it */
	ibv_free_device_list(dev_list);
	dev_list = NULL;
	ib_dev = NULL;

	/* query port properties  */
	if (ibv_query_port(res->ib_ctx, ib_port, &res->port_attr)) {
		fprintf(stderr, "ibv_query_port on port %u failed\n", ib_port);
		return 1;
	}

	/* allocate Protection Domain */
	res->pd = ibv_alloc_pd(res->ib_ctx);
	if (!res->pd) {
		fprintf(stderr, "ibv_alloc_pd failed\n");
		return 1;
	}

	//todo: change cq size
	cq_size = 1;
	res->cq = ibv_create_cq(res->ib_ctx, cq_size, NULL, NULL, 0);
	if (!res->cq) {
		fprintf(stderr, "failed to create CQ with %u entries\n", cq_size);
		return 1;
	}

	// todo: change memory region
	size = 1024;
	res->buf = malloc(size);
	if (!res->buf) {
		fprintf(stderr, "failed to malloc %Zu bytes to memory buffer\n", size);
		return 1;
	}

    // todo: change flags
	mr_flags = IBV_ACCESS_LOCAL_WRITE;
	res->mr = ibv_reg_mr(res->pd, res->buf, size, mr_flags);
	if (!res->mr) {
		fprintf(stderr, "ibv_reg_mr failed with mr_flags=0x%x\n", mr_flags);
		return 1;
	}

	fprintf(stdout, "MR was registered with addr=%p, lkey=0x%x, rkey=0x%x, flags=0x%x\n",
			      res->buf, res->mr->lkey, res->mr->rkey, mr_flags);

    /* create the Queue Pair */
	memset(&qp_init_attr, 0, sizeof(qp_init_attr));

	qp_init_attr.qp_type    	= IBV_QPT_RC;
	qp_init_attr.sq_sig_all 	= 1;
	qp_init_attr.send_cq    	= res->cq;
	qp_init_attr.recv_cq    	= res->cq;
	qp_init_attr.cap.max_send_wr  	= 1;
	qp_init_attr.cap.max_recv_wr  	= 1;
	qp_init_attr.cap.max_send_sge 	= 1;
	qp_init_attr.cap.max_recv_sge 	= 1;

	res->qp = ibv_create_qp(res->pd, &qp_init_attr);
	if (!res->qp) {
		fprintf(stderr, "failed to create QP\n");
		return 1;
	}
	fprintf(stdout, "QP was created, QP number=0x%x\n", res->qp->qp_num);

    return 0;
}