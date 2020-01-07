/*
 * Copyright IBM Corp. 2003,2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:		Sven Schuetz <sven@de.ibm.com>
 *
 * File:		vlib_sg_io.c
 *
 * Description:
 * All functions performing sg_io related stuff.
 *
 */

#include "vlib.h"

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>

#include <linux/bsg.h>
#include <scsi/fc/fc_els.h>
#include <scsi/fc/fc_gs.h>
#include <scsi/fc/fc_ns.h>
#include <scsi/scsi_bsg_fc.h>

struct gid_pn_req_frame {
	struct fc_ct_hdr hdr;
	struct fc_ns_gid_pn gid_pn_req;
};

struct gid_pn_rsp_frame {
	struct fc_ct_hdr hdr;
	struct fc_gid_pn_resp gid_pn_rsp;
};

static int sg_io_performSGIO(char *devName, struct sg_io_v4 *sg_io)
{
	int fd;
	int status;
	int i;

	fd = open(devName, O_RDWR);
	if (fd < 0)
		return fd;

	status = ioctl(fd, SG_IO, sg_io);

	if (status < 0)
		return status;

	status = close(fd);

	if (status < 0)
		return status;

	return 0;
}

HBA_STATUS sg_io_performCTPassThru(struct vlib_adapter *adapter,
				void* req, int reqSize, void* rsp, int rspSize)
{
	char devName[PATH_MAX];
	struct fc_bsg_request cdb;
	struct sg_io_v4 sg_io;


	bzero(&cdb, sizeof(cdb));
	bzero(&sg_io, sizeof(sg_io));
	snprintf(devName, PATH_MAX, "/dev/bsg/fc_host%d", adapter->ident.host);
	cdb.msgcode = FC_BSG_HST_CT;
	memcpy(&cdb.rqst_data.r_ct, req, sizeof(struct fc_bsg_rport_ct));
						/* copy the preamble into the
						request header for the LLD */

	sg_io.guard = 'Q';
	sg_io.protocol = BSG_PROTOCOL_SCSI;
	sg_io.subprotocol = BSG_SUB_PROTOCOL_SCSI_TRANSPORT;

	sg_io.request_len = sizeof(cdb);
	sg_io.request = (__u64) &cdb;
	sg_io.dout_xfer_len = reqSize;
	sg_io.dout_xferp = (__u64) req;
	sg_io.din_xfer_len = rspSize;
	sg_io.din_xferp = (__u64) rsp;

	sg_io.timeout = 9000; /* TODO */

	if (sg_io_performSGIO(devName, &sg_io))
		return HBA_STATUS_ERROR;

	return HBA_STATUS_OK;
}
							
static HBA_STATUS sg_io_performGIDPN(struct vlib_adapter *adapter,
						wwn_t portwwn, void *rsp)
{
	char devName[PATH_MAX];
	struct fc_bsg_request cdb;
	struct gid_pn_req_frame ct;

	struct sg_io_v4 sg_io;

	snprintf(devName, PATH_MAX, "/dev/bsg/fc_host%d", adapter->ident.host);

	memset(&ct, 0, sizeof(struct gid_pn_req_frame));
	ct.hdr.ct_rev = 1;
	ct.hdr.ct_fs_type = 0xFC;
	ct.hdr.ct_fs_subtype = 0x02;
	ct.hdr.ct_cmd = 0x0121;          /* gid_pn = 0x0112 */
	ct.hdr.ct_mr_size = 0x1;  /* response should be one word */

	memcpy(&ct.gid_pn_req.fn_wwpn, &portwwn, sizeof(wwn_t));

	cdb.msgcode = FC_BSG_HST_CT;
	memcpy(&cdb.rqst_data.r_ct, &ct, sizeof(struct fc_bsg_rport_ct));
						/* copy the preamble into the
						request header for the LLD */
	sg_io.guard = 'Q';
	sg_io.protocol = BSG_PROTOCOL_SCSI;
	sg_io.subprotocol = BSG_SUB_PROTOCOL_SCSI_TRANSPORT;

	sg_io.request_len = sizeof(cdb);
	sg_io.request = (__u64) &cdb;
	sg_io.dout_xfer_len = sizeof(ct);
	sg_io.dout_xferp = (__u64) &ct;
	sg_io.din_xfer_len = CT_GIDPN_RESPONSE_LENGTH;
	sg_io.din_xferp = (__u64) rsp;

	sg_io.timeout = 2000; /* TODO */

	if (sg_io_performSGIO(devName, &sg_io))
		return HBA_STATUS_ERROR;

	return HBA_STATUS_OK;
}

static fc_id_t getDidFromWWN(struct vlib_adapter *adapter, wwn_t portwwn)
{
	struct gid_pn_rsp_frame rsp;
	fc_id_t d_id;

	if (sg_io_performGIDPN(adapter, portwwn, &rsp))
		;

	if (rsp.hdr.ct_cmd == 0x8002) {
		/* accept CT */
		/* convert from u8[3] to 32bit number*/
		return *((fc_id_t*)&rsp.gid_pn_rsp.fp_fid);
	}

	/* all other cases including reject ct */
	return 0;
}

HBA_STATUS sg_io_sendRNID(struct vlib_adapter *adapter, wwn_t portwwn,
							void *rsp, int rspSize)
{
	char devName[PATH_MAX];
	struct fc_bsg_request cdb;
	struct fc_els_rnid rnid;
	struct sg_io_v4 sg_io;
	fc_id_t d_id;
	int i;

	d_id = getDidFromWWN(adapter, portwwn);
	if (d_id == 0)
		return HBA_STATUS_ERROR;

	memset(&rnid, 0, sizeof(struct fc_els_rnid));
	memset(&cdb, 0, sizeof(struct fc_bsg_request));
	rnid.rnid_cmd = ELS_RNID;
	rnid.rnid_fmt = ELS_RNIDF_GEN;
	cdb.msgcode = FC_BSG_HST_ELS_NOLOGIN;
	cdb.rqst_data.h_els.command_code = 0x78;
	memcpy(cdb.rqst_data.h_els.port_id, (char *)&d_id,
					sizeof(cdb.rqst_data.h_els.port_id));

	sg_io.guard = 'Q';
	sg_io.protocol = BSG_PROTOCOL_SCSI;
	sg_io.subprotocol = BSG_SUB_PROTOCOL_SCSI_TRANSPORT;
	sg_io.request_len = sizeof(cdb);
	sg_io.request = (__u64) &cdb;
	sg_io.dout_xfer_len = sizeof(struct fc_els_rnid);
	sg_io.dout_xferp = (__u64) &rnid;
	sg_io.din_xfer_len = rspSize; /* common node-identification data + 4 */
	sg_io.din_xferp = (__u64) rsp;

	sg_io.timeout = 5000;

	sprintf(devName, "/dev/bsg/fc_host%d", adapter->ident.host);

	memset(rsp, 0, rspSize);

	if (sg_io_performSGIO(devName, &sg_io))
		return HBA_STATUS_ERROR;

	return HBA_STATUS_OK;
}

#if 0
static void prepareRPS(wwn_t *port_selection, char *flag, fc_id_t *d_id,
				wwn_t agent_wwn, HBA_UINT32 agent_domain,
				wwn_t object_wwn, HBA_UINT32 object_port_number)
{
	if (agent_wwn)
		/* there is a target port */
		*d_id = getDidFromWWN(adapter, agent_wwn);
	else
		/* no target port, so we pick the domain controller */
		*d_id = 0xFFFC00 | agent_domain;

	if (object_wwn) {
		/* the port in question is identified by a wwpn */
		*flag = 0x02;
		*port_selection = object_wwn;
	} else {
		/* the port in question is identified by a physical port no */
		*flag = 0x01;
		*port_selection = object_port_number;
	}
}
#endif

/*HBA_STATUS sg_io_sendRPS(struct vlib_adapter *adapter,
				wwn_t agent_wwn, HBA_UINT32 agent_domain,
				wwn_t object_wwn, HBA_UINT32 object_port_number,
				void *pRspBuffer, HBA_UINT32 *pRspBufferSize)
{
	fc_id_t d_id;
	wwn_t port_selection;
	char flag;

	prepareRPS(&port_selection, &flag, &d_id, agent_wwn, agent_domain,
						object_wwn, object_port_number);

}*/
