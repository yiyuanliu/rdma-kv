#include "ib.h"
#include <stdio.h>

int create_ib_res(ib_res* res, uint8_t ib_port) {
    struct ibv_device **dev_list = NULL;
    struct ibv_qp_init_attr qp_init_attr;
    struct ibv_device *ib_dev = NULL;
    int i;
    int mr_flags = 0;
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
    dev_name = strdup(ibv_get_device_name(dev_list[0])); 
	fprintf(stdout, "using first one found: %s\n", dev_name);
	ib_dev = dev_list[0];
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

	res->cq = ibv_create_cq(res->ib_ctx, CQ_SIZE, NULL, NULL, 0);
	if (!res->cq) {
		fprintf(stderr, "failed to create CQ with %u entries\n", CQ_SIZE);
		return 1;
	}

    if (res->send_buf && res->send_buf_size) {
		mr_flags = 0;
		res->send_mr = ibv_reg_mr(res->pd, res->send_buf, res->send_buf_size, mr_flags);
		if (!res->send_mr) {
			fprintf(stderr, "ibv_reg_mr failed with mr_flags=0x%x\n", mr_flags);
			return 1;
		}
		fprintf(stdout, "send buffer was registered with addr=%p, lkey=0x%x, rkey=0x%x, flags=0x%x\n",
			      res->send_buf, res->send_mr->lkey, res->send_mr->rkey, mr_flags);
	}

	if (res->recv_buf && res->recv_buf_size) {
		mr_flags = IBV_ACCESS_LOCAL_WRITE;
		res->recv_mr = ibv_reg_mr(res->pd, res->recv_buf, res->recv_buf_size, mr_flags);
		if (!res->recv_buf) {
			fprintf(stderr, "ibv_reg_mr failed with mr_flags=0x%x\n", mr_flags);
			return 1;
		}
		fprintf(stdout, "recv buffer was registered with addr=%p, lkey=0x%x, rkey=0x%x, flags=0x%x\n",
			      res->recv_buf, res->recv_mr->lkey, res->recv_mr->rkey, mr_flags);
	}

    /* create the Queue Pair */
	memset(&qp_init_attr, 0, sizeof(qp_init_attr));

	qp_init_attr.qp_type    	= IBV_QPT_RC;
	qp_init_attr.sq_sig_all 	= 1;
	qp_init_attr.send_cq    	= res->cq;
	qp_init_attr.recv_cq    	= res->cq;
	qp_init_attr.cap.max_send_wr  	= MAX_SEND_WR;
	qp_init_attr.cap.max_recv_wr  	= MAX_RECV_WR;
	qp_init_attr.cap.max_send_sge 	= MAX_SEND_SGE;
	qp_init_attr.cap.max_recv_sge 	= MAX_RECV_SGE;

	res->qp = ibv_create_qp(res->pd, &qp_init_attr);
	if (!res->qp) {
		fprintf(stderr, "failed to create QP\n");
		return 1;
	}
	fprintf(stdout, "QP was created, QP number=0x%x\n", res->qp->qp_num);

    return 0;
}

int modify_qp_to_init(struct ibv_qp *qp, uint8_t ib_port) {
	struct ibv_qp_attr 	attr;
	int 			flags;
	int 			rc;

	/* do the following QP transition: RESET -> INIT */
	memset(&attr, 0, sizeof(attr));

	attr.qp_state 	= IBV_QPS_INIT;
	attr.port_num 	= ib_port;
	attr.pkey_index = 0;
	// todo: change this to allow rdma op.
	attr.qp_access_flags = 0;

	flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;

	rc = ibv_modify_qp(qp, &attr, flags);
	if (rc) {
		perror("modify_qp_to_init");
		return rc;
	}

	return 0;
}

int modify_qp_to_rtr(struct ibv_qp *qp, uint8_t ib_port, uint32_t dest_qpn, uint16_t dest_lid) {
	
	struct ibv_qp_attr attr;
	int flags;
	int rc;

	memset(&attr, 0, sizeof(attr));

	attr.qp_state = IBV_QPS_RTR;
	attr.path_mtu = IBV_MTU_256;
	attr.dest_qp_num = dest_qpn;
	attr.rq_psn = 0;
	attr.max_dest_rd_atomic = 0;
	attr.min_rnr_timer = 0x12;
	attr.ah_attr.is_global = 0;
	attr.ah_attr.dlid = dest_lid;
	attr.ah_attr.sl = 0;
	attr.ah_attr.src_path_bits = 0;
	attr.ah_attr.port_num = ib_port;

	flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN | IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;

	rc = ibv_modify_qp(qp, &attr, flags);
	if (rc) {
		perror("modify_qp_to_rtr");
		return rc;
	}

	return 0;
}

int modify_qp_to_rts(struct ibv_qp *qp) {
	struct ibv_qp_attr attr;
	int flags;
	int rc;

	memset(&attr, 0, sizeof(attr));

	attr.qp_state = IBV_QPS_RTS;
	attr.timeout = 0x12;
	attr.retry_cnt = 6;
	attr.rnr_retry = 0;
	attr.sq_psn = 0;
	attr.max_rd_atomic = 0;

 	flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

	rc = ibv_modify_qp(qp, &attr, flags);
	if (rc) {
		perror("modify_qp_to_rts");
		return rc;
	}

	return 0;
}

int destroy_ib_res(struct ib_res *res) {
	int test_result = 0;

	if (res->qp) {
		if (ibv_destroy_qp(res->qp)) {
			fprintf(stderr, "failed to destroy QP\n");
			test_result = 1;
		}
	}

	if (res->send_mr) {
		if (ibv_dereg_mr(res->send_mr)) {
			fprintf(stderr, "failed to deregister send MR\n");
			test_result = 1;
		}
	}

	if (res->send_buf)
		free(res->send_buf);

	if (res->recv_mr) {
		if (ibv_dereg_mr(res->recv_mr)) {
			fprintf(stderr, "failed to deregister recv MR\n");
			test_result = 1;
		}
	}

	if (res->recv_buf) {
		free(res->recv_buf);
	}

	if (res->cq) {
		if (ibv_destroy_cq(res->cq)) {
			fprintf(stderr, "failed to destroy CQ\n");
			test_result = 1;
		}
	}

	if (res->pd) {
		if (ibv_dealloc_pd(res->pd)) {
			fprintf(stderr, "failed to deallocate PD\n");
			test_result = 1;
		}
	}

	if (res->ib_ctx) {
		if (ibv_close_device(res->ib_ctx)) {
			fprintf(stderr, "failed to close device context\n");
			test_result = 1;
		}
	}
	return test_result;
}

int post_receive(struct ib_res *res) {
	struct ibv_recv_wr 	rr;
	struct ibv_sge 		sge;
	struct ibv_recv_wr 	*bad_wr;
	int rc;

	/* prepare the scatter/gather entry */
	memset(&sge, 0, sizeof(sge));
	sge.addr = (uintptr_t)res->recv_buf;
	sge.length = res->recv_buf_size;
	sge.lkey = res->recv_mr->lkey;

	/* prepare the receive work request (RR) */
	memset(&rr, 0, sizeof(rr));

	rr.next 	= NULL;
	rr.wr_id 	= 0;
	rr.sg_list 	= &sge;
	rr.num_sge 	= 1;

	/* post the Receive Request to the RQ */
	rc = ibv_post_recv(res->qp, &rr, &bad_wr);
	if (rc) {
		fprintf(stderr, "failed to post RR\n");
		return 1;
	}

	fprintf(stdout, "Receive Request was posted\n");

	return 0;
}