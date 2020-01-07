/*
 * Copyright IBM Corp. 2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:		Sven Schuetz <sven@de.ibm.com>
 *
 * File:		vlib_sg.c
 *
 * Description:
 * All calls that use sg_utils library.
 *
 */
 
 /**
 * @file vlib_sg.c
 * @brief All calls that use sg_utils library.
 */

#include "vlib.h"

#define INTERVAL	10000000
#define RETRIES		1500

HBA_STATUS sgutils_SendScsiInquiry(char* sg_dev, HBA_UINT8 EVPD, 
				HBA_UINT32 PageCode, void *pRspBuffer,
				HBA_UINT32 *RspBufferSize)
{
	char dev_path[32];
	int sg_fd, res;

	if(sg_dev == NULL)
		return HBA_STATUS_ERROR;

	strcpy(dev_path, "/dev/");
	strcat(dev_path, sg_dev);

	sg_fd = sg_cmds_open_device(dev_path, 0, 0);
	if (sg_fd < 0)
		return HBA_STATUS_ERROR;
	memset(pRspBuffer, 0x0, *RspBufferSize);
	res = sg_ll_inquiry(sg_fd, 0, EVPD, PageCode, pRspBuffer,
							*RspBufferSize, 0, 0);
	if(res < 0) {
		sg_cmds_close_device(sg_fd);
		return HBA_STATUS_ERROR;
	}

	res = sg_cmds_close_device(sg_fd);
	if(res < 0)
		return HBA_STATUS_ERROR;

	return HBA_STATUS_OK;
}

HBA_STATUS sgutils_SendReportLUNs(char* sg_dev, char *pRspBuffer,
				HBA_UINT32 *RspBufferSize)
{
	int sg_fd, res, count = 0;
	char dev_path[32];
	struct timespec t;
	int size;
	HBA_STATUS status;

	status = HBA_STATUS_OK;

	strcpy(dev_path, "/dev/");
	if(sg_dev == NULL)
		return HBA_STATUS_ERROR;

	strcat(dev_path, sg_dev);

	t.tv_nsec = INTERVAL;
	t.tv_sec = 0;
	sg_fd = sg_cmds_open_device(dev_path, 0, 0);
	while (sg_fd < 0 && count < RETRIES) {
		nanosleep(&t, NULL);
		sg_fd = sg_cmds_open_device(dev_path, 0, 0);
		count++;
	}
	if (sg_fd < 0)
		return HBA_STATUS_ERROR;

	memset(pRspBuffer, 0x0, *RspBufferSize);
	res = sg_ll_report_luns(sg_fd, 0, pRspBuffer, *RspBufferSize, 0, 0);
	if(res < 0) {
		sg_cmds_close_device(sg_fd);
		return HBA_STATUS_ERROR;
	}

	size = ((pRspBuffer[0] << 24) | (pRspBuffer[1] << 16) |
					(pRspBuffer[2] << 8) | pRspBuffer[3]);

	/* size is size of payload, total response size is 8 bytes larger
	 * because of the header */
	if (*RspBufferSize < (size + 8))
		status = HBA_STATUS_ERROR_MORE_DATA;
	else
		*RspBufferSize = size + 8;

	res = sg_cmds_close_device(sg_fd);
	if(res < 0)
		return HBA_STATUS_ERROR;

	return status;
}

HBA_STATUS sgutils_SendReadCap(char* sg_dev, char *pRspBuffer,
				HBA_UINT32 *RspBufferSize)
{
	char dev_path[32];
	int sg_fd, res, blocks;
	HBA_STATUS status;

	status = HBA_STATUS_OK;

	if(sg_dev == NULL)
		return HBA_STATUS_ERROR;
	
	strcpy(dev_path, "/dev/");
	strcat(dev_path, sg_dev);

	sg_fd = sg_cmds_open_device(dev_path, 0, 0);
	if (sg_fd < 0)
		return HBA_STATUS_ERROR;

	memset(pRspBuffer, 0x0, *RspBufferSize);
	res = sg_ll_readcap_10(sg_fd, 0, 0, pRspBuffer, READCAP10LEN, 0, 0);
	if(res < 0) {
		sg_cmds_close_device(sg_fd);
		return HBA_STATUS_ERROR;
	}

	blocks = ((pRspBuffer[0] << 24) | (pRspBuffer[1] << 16) |
					(pRspBuffer[2] << 8) | pRspBuffer[3]);

	*RspBufferSize = READCAP10LEN;
	if (blocks != 0xffffffff)
		/* device smaller than 0xffffffff blocks, reply ok */
		return HBA_STATUS_OK;

	/* device larger than 0xffffffff, readcap16 necessary */
	if (*RspBufferSize < READCAP16LEN) {
		/* buffer too small to hold the smallest response */
		status = HBA_STATUS_ERROR_MORE_DATA;
		goto out;
	}

	*RspBufferSize = READCAP16LEN;

	res = sg_ll_readcap_16(sg_fd, 0, 0, pRspBuffer, READCAP10LEN, 0, 0);
	if(res < 0) {
		sg_cmds_close_device(sg_fd);
		return HBA_STATUS_ERROR;
	}

out:
	res = sg_cmds_close_device(sg_fd);
	if(res < 0)
		return HBA_STATUS_ERROR;

	return status;
}
