/*
 * Copyright IBM Corp. 2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:		Sven Schuetz <sven@de.ibm.com>
 *
 * File:		vlib_sg.h
 *
 * Description:
 * Function declarations for functions using the sg_utils library
 *
 */

#ifndef _VLIB_SG_H_
#define _VLIB_SG_H_

#define READCAP10LEN 8
#define READCAP16LEN 32

HBA_STATUS sgutils_SendScsiInquiry(char *sg_dev, HBA_UINT8 EVPD,
				HBA_UINT32 PageCode, void *pRspBuffer,
				HBA_UINT32 *RspBufferSize);
HBA_STATUS sgutils_SendReportLUNs(char *sg_dev, char *pRspBuffer,
				HBA_UINT32 *RspBufferSize);
HBA_STATUS sgutils_SendReadCap(char *sg_dev, char *pRspBuffer,
				HBA_UINT32 *RspBufferSize);

#endif /*_VLIB_SG_H_*/
