/*
 * Copyright IBM Corp. 2003, 2004, 2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:	Andreas Herrmann
 *		Stefan Voelkel
 *
 * File:	hbaapi.h
 *
 * Description:
 * HBA API header file (according to FC-HBA Rev 11)
 *
 */

#ifndef _HBAAPI_H_
#define _HBAAPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <time.h>

/*
 * basic attribute types
 */
typedef uint8_t HBA_UINT8;
typedef uint16_t HBA_UINT16;
typedef uint32_t HBA_UINT32;
typedef uint64_t HBA_UINT64;
typedef int64_t HBA_INT64;
typedef HBA_UINT8 HBA_BOOLEAN;
typedef HBA_UINT32 HBA_HANDLE;

typedef struct HBA_wwn {
	HBA_UINT8 wwn[8];
} HBA_WWN;

/*
 * status return values
 */
typedef HBA_UINT32 HBA_STATUS;

#define HBA_STATUS_OK				0
#define HBA_STATUS_ERROR			1
#define HBA_STATUS_ERROR_NOT_SUPPORTED		2
#define HBA_STATUS_ERROR_INVALID_HANDLE		3
#define HBA_STATUS_ERROR_ARG			4
#define HBA_STATUS_ERROR_ILLEGAL_WWN		5
#define HBA_STATUS_ERROR_ILLEGAL_INDEX		6
#define HBA_STATUS_ERROR_MORE_DATA		7
#define HBA_STATUS_ERROR_STALE_DATA		8
#define HBA_STATUS_SCSI_CHECK_CONDITION		9
#define HBA_STATUS_ERROR_BUSY			10
#define HBA_STATUS_ERROR_TRY_AGAIN		11
#define HBA_STATUS_ERROR_UNAVAILABLE		12
#define HBA_STATUS_ERROR_ELS_REJECT		13
#define HBA_STATUS_ERROR_INVALID_LUN		14
#define HBA_STATUS_ERROR_INCOMPATIBLE		15
#define HBA_STATUS_ERROR_AMBIGUOUS_WWN		16
#define HBA_STATUS_ERROR_LOCAL_BUS		17
#define HBA_STATUS_ERROR_LOCAL_TARGET		18
#define HBA_STATUS_ERROR_LOCAL_LUN		19
#define HBA_STATUS_ERROR_LOCAL_SCSIID_BOUND	20
#define HBA_STATUS_ERROR_TARGET_FCID		21
#define HBA_STATUS_ERROR_TARGET_NODE_WWN	22
#define HBA_STATUS_ERROR_TARGET_PORT_WWN	23
#define HBA_STATUS_ERROR_TARGET_LUN		24
#define HBA_STATUS_ERROR_TARGET_LUID		25
#define HBA_STATUS_ERROR_NO_SUCH_BINDING	26
#define HBA_STATUS_ERROR_NOT_A_TARGET		27
#define HBA_STATUS_ERROR_UNSUPPORTED_FC4	28
#define HBA_STATUS_ERROR_INCAPABLE		29
#define HBA_STATUS_ERROR_TARGET_BUSY		30
#define HBA_STATUS_ERROR_NOT_LOADED		31
#define HBA_STATUS_ERROR_ALREADY_LOADED		32
#define HBA_STATUS_ERROR_ILLEGAL_FCID		33

/*
 * HBA attributes
 */
typedef struct HBA_AdapterAttributes {
	char Manufacturer[64];
	char SerialNumber[64];
	char Model[256];
	char ModelDescription[256];
	HBA_WWN NodeWWN;
	char NodeSymbolicName[256];
	char HardwareVersion[256];
	char DriverVersion[256];
	char OptionROMVersion[256];
	char FirmwareVersion[256];
	HBA_UINT32 VendorSpecificID;
	HBA_UINT32 NumberOfPorts;
	char DriverName[256];
} HBA_ADAPTERATTRIBUTES;

/*
 * FC port attributes
 */
typedef HBA_UINT32 HBA_PORTTYPE;

#define HBA_PORTTYPE_UNKNOWN		1
#define HBA_PORTTYPE_OTHER		2
#define HBA_PORTTYPE_NOTPRESENT		3
#define HBA_PORTTYPE_NPORT		5
#define HBA_PORTTYPE_NLPORT		6
#define HBA_PORTTYPE_FLPORT		7
#define HBA_PORTTYPE_FPORT		8
#define HBA_PORTTYPE_LPORT		20
#define HBA_PORTTYPE_PTP		21

typedef HBA_UINT32 HBA_PORTSTATE;

#define HBA_PORTSTATE_UNKNOWN		1
#define HBA_PORTSTATE_ONLINE		2
#define HBA_PORTSTATE_OFFLINE		3
#define HBA_PORTSTATE_BYPASSED		4
#define HBA_PORTSTATE_DIAGNOSTICS	5
#define HBA_PORTSTATE_LINKDOWN		6
#define HBA_PORTSTATE_ERROR		7
#define HBA_PORTSTATE_LOOPBACK		8

typedef HBA_UINT32 HBA_PORTSPEED;

#define HBA_PORTSPEED_UNKNOWN		0
#define HBA_PORTSPEED_1GBIT		1
#define HBA_PORTSPEED_2GBIT		2
#define HBA_PORTSPEED_10GBIT		4
#define HBA_PORTSPEED_4GBIT		8
#define HBA_PORTSPEED_8GBIT		16
#define HBA_PORTSPEED_16GBIT		32
#define HBA_PORTSPEED_NOT_NEGOTIATED	(1<<15)

/* "Class of Service - Format" as described in FC-GS-4 */
typedef HBA_UINT32 HBA_COS;

/* "FC-TYPEs - Format" as described in FC-GS-4 */
typedef struct HBA_fc4types {
	HBA_UINT8 bits[32];
} HBA_FC4TYPES;

typedef struct HBA_PortAttributes {
	HBA_WWN NodeWWN;
	HBA_WWN PortWWN;
	HBA_UINT32 PortFcId;
	HBA_PORTTYPE PortType;
	HBA_PORTSTATE PortState;
	HBA_COS PortSupportedClassofService;
	HBA_FC4TYPES PortSupportedFc4Types;
	HBA_FC4TYPES PortActiveFc4Types;
	char PortSymbolicName[256];
	char OSDeviceName[256];
	HBA_PORTSPEED PortSupportedSpeed;
	HBA_PORTSPEED PortSpeed;
	HBA_UINT32 PortMaxFrameSize;
	HBA_WWN FabricName;
	HBA_UINT32 NumberofDiscoveredPorts;
} HBA_PORTATTRIBUTES;

/*
 * end port statistics
 */
/* for FC-1, FC-2, FC-3 */
typedef struct HBA_PortStatistics {
	HBA_INT64 SecondsSinceLastReset;
	HBA_INT64 TxFrames;
	HBA_INT64 TxWords;
	HBA_INT64 RxFrames;
	HBA_INT64 RxWords;
	HBA_INT64 LIPCount;
	HBA_INT64 NOSCount;
	HBA_INT64 ErrorFrames;
	HBA_INT64 DumpedFrames;
	HBA_INT64 LinkFailureCount;
	HBA_INT64 LossOfSyncCount;
	HBA_INT64 LossOfSignalCount;
	HBA_INT64 PrimitiveSeqProtocolErrCount;
	HBA_INT64 InvalidTxWordCount;
	HBA_INT64 InvalidCRCCount;
} HBA_PORTSTATISTICS;

/* for FC-4 */
typedef struct HBA_FC4Statistics {
	HBA_INT64 InputRequests;
	HBA_INT64 OutputRequests;
	HBA_INT64 ControlRequests;
	HBA_INT64 InputMegabytes;
	HBA_INT64 OutputMegabytes;
} HBA_FC4STATISTICS;

/*
 * FCP port attributes (FCP-2)
 */
typedef enum HBA_fcpbindingtype {
	TO_D_ID,
	TO_WWN,
	TO_OTHER
} HBA_FCPBINDINGTYPE;

typedef HBA_UINT32 HBA_BIND_CAPABILITY;

#define HBA_CAN_BIND_TO_D_ID		0x0001
#define HBA_CAN_BIND_TO_WWPN		0x0002
#define HBA_CAN_BIND_TO_WWNN		0x0004
#define HBA_CAN_BIND_TO_LUID		0x0008
#define HBA_CAN_BIND_ANY_LUNS		0x0400
#define HBA_CAN_BIND_TARGETS		0x0800
#define HBA_CAN_BIND_AUTOMAP		0x1000
#define HBA_CAN_BIND_CONFIGURED		0x2000

typedef HBA_UINT32 HBA_BIND_TYPE;

#define HBA_BIND_TO_D_ID		0x0001
#define HBA_BIND_TO_WWPN		0x0002
#define HBA_BIND_TO_WWNN		0x0004
#define HBA_BIND_TO_LUID		0x0008
#define HBA_BIND_TARGETS		0x8000

typedef struct HBA_LUID {
	char buffer[256];
} HBA_LUID;

typedef struct HBA_ScsiId {
	char OSDeviceName[256];
	HBA_UINT32 ScsiBusNumber;
	HBA_UINT32 ScsiTargetNumber;
	HBA_UINT32 ScsiOSLun;
} HBA_SCSIID;

typedef struct HBA_FcpId {
	HBA_UINT32 FcId;
	HBA_WWN NodeWWN;
	HBA_WWN PortWWN;
	HBA_UINT64 FcpLun;
} HBA_FCPID;

typedef struct HBA_FcpScsiEntry {
	HBA_SCSIID ScsiId;
	HBA_FCPID FcpId;
} HBA_FCPSCSIENTRY;

typedef struct HBA_FcpScsiEntryV2 {
	HBA_SCSIID ScsiId;
	HBA_FCPID FcpId;
	HBA_LUID LUID;
} HBA_FCPSCSIENTRYV2;

typedef struct HBA_FCPTargetMapping {
	HBA_UINT32 NumberOfEntries;
	HBA_FCPSCSIENTRY entry[1];	/* variable length array */
} HBA_FCPTARGETMAPPING;

typedef struct HBA_FCPTargetMappingV2 {
	HBA_UINT32 NumberOfEntries;
	HBA_FCPSCSIENTRYV2 entry[1];	/* variable length array */
} HBA_FCPTARGETMAPPINGV2;

typedef struct HBA_FCPBindingEntry {
	HBA_FCPBINDINGTYPE type;
	HBA_SCSIID ScsiId;
	HBA_FCPID FcpId;
	HBA_UINT32 FcId;
} HBA_FCPBINDINGENTRY;

typedef struct HBA_FCPBinding {
	HBA_UINT32 NumberOfEntries;
	HBA_FCPBINDINGENTRY entry[1];	/* variable length array */
} HBA_FCPBINDING;

typedef struct HBA_FCPBindingEntry2 {
	HBA_BIND_TYPE type;
	HBA_SCSIID ScsiId;
	HBA_FCPID FcpId;
	HBA_LUID LUID;
	HBA_STATUS Status;
} HBA_FCPBINDINGENTRY2;

typedef struct HBA_FCPBinding2 {
	HBA_UINT32 NumberOfEntries;
	HBA_FCPBINDINGENTRY2 entry[1];	/* variable length array */
} HBA_FCPBINDING2;

/*
 * FC-3 management attributes
 */
typedef enum HBA_wwntype {
	NODE_WWN,
	PORT_WWN
} HBA_WWNTYPE;

typedef struct HBA_MgmtInfo {
	HBA_WWN wwn;
	HBA_UINT32 unittype;
	HBA_UINT32 PortId;
	HBA_UINT32 NumberOfAttachedNodes;
	HBA_UINT16 IPVersion;
	HBA_UINT16 UDPPort;
	HBA_UINT8 IPAddress[16];
	HBA_UINT16 reserved;
	HBA_UINT16 TopologyDiscoveryFlags;
} HBA_MGMTINFO;

/*
 * polled event notification
 */
#define HBA_EVENT_LIP_OCCURRED		1
#define HBA_EVENT_LINK_UP		2
#define HBA_EVENT_LINK_DOWN		3
#define HBA_EVENT_LIP_RESET_OCCURRED	4
#define HBA_EVENT_RSCN			5
#define HBA_EVENT_PROPRIETARY		0xffff

typedef struct HBA_LinkEventInfo {
	HBA_UINT32 PortFcId;	/* end port where event occurred */
	HBA_UINT32 Reserved[3];
} HBA_LINK_EVENTINFO;

typedef struct HBA_RSCN_EventInfo {
	HBA_UINT32 PortFcId;	/* end port where event occurred */
	HBA_UINT32 NPortPage;
	HBA_UINT32 Reserved[2];
} HBA_RSCN_EVENTINFO;

typedef struct HBA_Pty_EventInfo {
	HBA_UINT32 PtyData[4];	/* proprietary data */
} HBA_PTY_EVENTINFO;

typedef struct HBA_EventInfo {
	HBA_UINT32 EventCode;
	union {
		HBA_LINK_EVENTINFO Link_EventInfo;
		HBA_RSCN_EVENTINFO RSCN_EventInfo;
		HBA_PTY_EVENTINFO Pty_EventInfo;
	} Event;
} HBA_EVENTINFO;

/*
 * asynchronous event notification
 */
typedef void *HBA_CALLBACKHANDLE;

/* adapter add event type */
#define HBA_EVENT_ADAPTER_ADD			0x101

/* adapter event types */
#define HBA_EVENT_ADAPTER_UNKNOWN		0x100
#define HBA_EVENT_ADAPTER_REMOVE		0x102
#define HBA_EVENT_ADAPTER_CHANGE		0x103

/* port event types */
#define HBA_EVENT_PORT_UNKNOWN			0x200
#define HBA_EVENT_PORT_OFFLINE			0x201
#define HBA_EVENT_PORT_ONLINE			0x202
#define HBA_EVENT_PORT_NEW_TARGETS		0x203
#define HBA_EVENT_PORT_FABRIC			0x204

/* port statistics event types */
#define HBA_EVENT_PORT_STAT_THRESHOLD		0x301
#define HBA_EVENT_PORT_STAT_GROWTH		0x302

/* target event types */
#define HBA_EVENT_TARGET_UNKNOWN		0x400
#define HBA_EVENT_TARGET_OFFLINE		0x401
#define HBA_EVENT_TARGET_ONLINE			0x402
#define HBA_EVENT_TARGET_REMOVED		0x403

/* link event types */
#define HBA_EVENT_LINK_UNKNOWN			0x500
#define HBA_EVENT_LINK_INCIDENT			0x501

/*
 * library attributes
 */
typedef struct HBA_LibraryAttributes {
	HBA_BOOLEAN final;
	char LibPath[256];
	char VName[256];
	char VVersion[256];
	struct tm build_date;
} HBA_LIBRARYATTRIBUTES;

/*
 * function declarations for HBA API
 */

/* library control */
HBA_UINT32 HBA_GetVersion(void);
HBA_STATUS HBA_LoadLibrary(void);
HBA_STATUS HBA_FreeLibrary(void);
HBA_UINT32 HBA_GetWrapperLibraryAttributes(HBA_LIBRARYATTRIBUTES *);
HBA_UINT32 HBA_GetVendorLibraryAttributes(HBA_UINT32, HBA_LIBRARYATTRIBUTES *);
HBA_UINT32 HBA_GetNumberOfAdapters(void);
void HBA_RefreshInformation(HBA_HANDLE);
void HBA_RefreshAdapterConfiguration(void);
void HBA_ResetStatistics(HBA_HANDLE, HBA_UINT32);

/* adapter and port information */
HBA_STATUS HBA_GetAdapterName(HBA_UINT32, char *);
HBA_HANDLE HBA_OpenAdapter(char *);
HBA_STATUS HBA_OpenAdapterByWWN(HBA_HANDLE *, HBA_WWN);
void HBA_CloseAdapter(HBA_HANDLE);
HBA_STATUS HBA_GetAdapterAttributes(HBA_HANDLE, HBA_ADAPTERATTRIBUTES *);
HBA_STATUS HBA_GetAdapterPortAttributes
	(HBA_HANDLE, HBA_UINT32, HBA_PORTATTRIBUTES *);
HBA_STATUS HBA_GetDiscoveredPortAttributes
	(HBA_HANDLE, HBA_UINT32, HBA_UINT32, HBA_PORTATTRIBUTES *);
HBA_STATUS HBA_GetPortAttributesByWWN
	(HBA_HANDLE, HBA_WWN, HBA_PORTATTRIBUTES *);
HBA_STATUS HBA_GetPortStatistics(HBA_HANDLE, HBA_UINT32, HBA_PORTSTATISTICS *);
HBA_STATUS HBA_GetFC4Statistics
	(HBA_HANDLE, HBA_WWN, HBA_UINT8, HBA_FC4STATISTICS *);

/* FCP information */
HBA_STATUS HBA_GetBindingCapability(HBA_HANDLE, HBA_WWN, HBA_BIND_CAPABILITY *);
HBA_STATUS HBA_GetBindingSupport(HBA_HANDLE, HBA_WWN, HBA_BIND_CAPABILITY *);
HBA_STATUS HBA_SetBindingSupport(HBA_HANDLE, HBA_WWN, HBA_BIND_CAPABILITY);
HBA_STATUS HBA_GetFcpTargetMapping(HBA_HANDLE, HBA_FCPTARGETMAPPING *);
HBA_STATUS HBA_GetFcpTargetMappingV2
	(HBA_HANDLE, HBA_WWN, HBA_FCPTARGETMAPPINGV2 *);
HBA_STATUS HBA_GetFcpPersistentBinding(HBA_HANDLE, HBA_FCPBINDING *);
HBA_STATUS HBA_GetPersistentBindingV2(HBA_HANDLE, HBA_WWN, HBA_FCPBINDING2 *);
HBA_STATUS HBA_SetPersistentBindingV2
	(HBA_HANDLE, HBA_WWN, HBA_FCPBINDING2 *);
HBA_STATUS HBA_RemovePersistentBinding
	(HBA_HANDLE, HBA_WWN, HBA_FCPBINDING2 *);
HBA_STATUS HBA_RemoveAllPersistentBindings(HBA_HANDLE, HBA_WWN);
HBA_STATUS HBA_GetFCPStatistics
	(HBA_HANDLE, const HBA_SCSIID *, HBA_FC4STATISTICS *);

/* SCSI information */
HBA_STATUS HBA_SendScsiInquiry
	(HBA_HANDLE, HBA_WWN, HBA_UINT64, HBA_UINT8, HBA_UINT32, void *,
	 HBA_UINT32, void *, HBA_UINT32);
HBA_STATUS HBA_ScsiInquiryV2
	(HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT64, HBA_UINT8, HBA_UINT8, void *,
	 HBA_UINT32 *, HBA_UINT8 *, void *, HBA_UINT32 *);
HBA_STATUS HBA_SendReportLUNs
	(HBA_HANDLE, HBA_WWN, void *, HBA_UINT32, void *, HBA_UINT32);
HBA_STATUS HBA_ScsiReportLUNsV2
	(HBA_HANDLE, HBA_WWN, HBA_WWN, void *, HBA_UINT32 *, HBA_UINT8 *,
	 void *, HBA_UINT32 *);
HBA_STATUS HBA_SendReadCapacity
	(HBA_HANDLE, HBA_WWN, HBA_UINT64, void *, HBA_UINT32, void *,
	 HBA_UINT32);
HBA_STATUS HBA_ScsiReadCapacityV2
	(HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT64, void *, HBA_UINT32 *,
	 HBA_UINT8 *, void *, HBA_UINT32 *);

/* fabric management */
HBA_STATUS HBA_SendCTPassThru
	(HBA_HANDLE, void *, HBA_UINT32, void *, HBA_UINT32);
HBA_STATUS HBA_SendCTPassThruV2
	(HBA_HANDLE, HBA_WWN, void *, HBA_UINT32, void *, HBA_UINT32 *);
HBA_STATUS HBA_SetRNIDMgmtInfo(HBA_HANDLE, HBA_MGMTINFO *);
HBA_STATUS HBA_GetRNIDMgmtInfo(HBA_HANDLE, HBA_MGMTINFO *);
HBA_STATUS HBA_SendRNID(HBA_HANDLE, HBA_WWN, HBA_WWNTYPE, void *, HBA_UINT32 *);
HBA_STATUS HBA_SendRNIDV2
	(HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT32, HBA_UINT32, void *,
	 HBA_UINT32 *);
HBA_STATUS HBA_SendRPL
	(HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT32, HBA_UINT32, void *,
	 HBA_UINT32 *);
HBA_STATUS HBA_SendRPS
	(HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT32, HBA_WWN, HBA_UINT32, void *,
	 HBA_UINT32 *);
HBA_STATUS HBA_SendSRL
	(HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT32, void *, HBA_UINT32 *);
HBA_STATUS HBA_SendLIRR
	(HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT8, HBA_UINT8, void *,
	 HBA_UINT32 *);
HBA_STATUS HBA_SendRLS(HBA_HANDLE, HBA_WWN, HBA_WWN, void *, HBA_UINT32 *);

/* event handling */
HBA_STATUS HBA_GetEventBuffer(HBA_HANDLE, HBA_EVENTINFO *, HBA_UINT32 *);
HBA_STATUS HBA_RegisterForAdapterAddEvents
	(void (*)(void *, HBA_WWN, HBA_UINT32), void *, HBA_CALLBACKHANDLE *);
HBA_STATUS HBA_RegisterForAdapterEvents
	(void (*)(void *, HBA_WWN, HBA_UINT32), void *, HBA_HANDLE,
	 HBA_CALLBACKHANDLE *);
HBA_STATUS HBA_RegisterForAdapterPortEvents
	(void (*)(void *, HBA_WWN, HBA_UINT32, HBA_UINT32), void *, HBA_HANDLE,
	 HBA_WWN, HBA_CALLBACKHANDLE *);
HBA_STATUS HBA_RegisterForAdapterPortStatEvents
	(void (*)(void *, HBA_WWN, HBA_UINT32), void *, HBA_HANDLE, HBA_WWN,
	 HBA_PORTSTATISTICS, HBA_UINT32, HBA_CALLBACKHANDLE *);
HBA_STATUS HBA_RegisterForTargetEvents
	(void (*)(void *, HBA_WWN, HBA_WWN, HBA_UINT32), void *, HBA_HANDLE,
	 HBA_WWN, HBA_WWN, HBA_CALLBACKHANDLE *, HBA_UINT32);
HBA_STATUS HBA_RegisterForLinkEvents
	(void (*)(void *, HBA_WWN, HBA_UINT32, void *, HBA_UINT32), void *,
	 void *, HBA_UINT32, HBA_HANDLE, HBA_CALLBACKHANDLE *);
HBA_STATUS HBA_RemoveCallback(HBA_CALLBACKHANDLE);

#ifdef __cplusplus
}
#endif

#endif /* _HBAAPI_H_ */
