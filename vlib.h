/*
 * Copyright IBM Corp. 2003,2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:		Andreas Herrmann
 *			Stefan Voelkel
 *			Sven Schuetz <sven@de.ibm.com>
 *
 * File:		vlib.h
 *
 * Description:
 * Contains typedefs, structure definitions, some inline functions and macros
 * needed in the library.
 *
 */

#ifndef _VLIB_H_
#define _VLIB_H_

/**
 * @file vlib.h
 * @brief Central header file for the library.
 *
 * Contains typedefs, structure definitions, some inline functions and macros
 * needed in the library.
 */

/**
 * @file config.h
 * @brief Configuration header file after configure script was run.
 *
 * Depending on selected configuration options, the header file might
 * contain the following macros.
 *	- VERSION (version of the package)
 *	- HBAAPI_VENDOR_LIB (used to compile library as a vendor specific
 *	library)
 *	- HBAAPI_BACKTRACE (used to enable backtraces in certain error
 *	situations)
 */

/**
 * @file hbaapi.h
 * @brief C header file descriping the HBA API.
 *
 * Contains function declarations, macro definitions etc. defined in @ref FCHBA
 */

/**
 * @mainpage ZFCP HBA API Library
 *
 * @section introduction Introduction
 *
 * ZFCP HBA API Library is part of an implementation of @ref FCHBA for the ZFCP
 * device driver. The ZFCP device driver is a FCP device driver for Linux
 * on zSeries. The whole implementation of @ref FCHBA for the ZFCP device driver
 * is called ZFCP HBA API. Its implementation consists of ZFCP HBA API Library -
 * a shared library which provides the API defined in @ref FCHBA.
 *
 *
 * @section compatibility Compatibility
 *
 * ZFCP HBA API Library supports the API functions listed in
 * @ref SupportedHBAAPIs. All other API functions of @ref FCHBA return
 * the status HBA_STATUS_ERROR_NOT_SUPPORTED if possible and are listed in
 * @ref UnSupportedHBAAPIs.
 *
 *
 * @section restriction Restrictions and Peculiarities
 *
 * For ZFCP HBA API the following restrictions and pecularities apply:
 *
 *	- Only adapters, ports and units can be accessed that are configured in
 *	the ZFCP device dirver. This is due to internal restrictions of
 *	the ZFCP device driver and due to the fact that the adapter might be
 *	shared between several VM guests.
 *	- If SCSI command REPORT LUNS is send to a target port a unit with
 *	FCP LUN 0xc101000000000000 is implicitly created if not yet existent.
 * 	Lifetime of that implicitly created unit is temporary. It is the
 * 	report luns "well known lun".
 *	- The function HBA_GetFcpTargetMapping() does not return an OSDeviceName
 *	in struct HBA_FCPTargetMapping. This is conform to @ref FCHBA since this
 *	field is optional.
 *	- Because the ZFCP device driver does not support Single Byte Command
 *	Code Sets Connections, the functions HBA_GetSBTargetMapping(),
 *	HBA_GetSBStatistics() and HBA_SBDskGetCapacity() are not supported
 *	by ZFCP HBA API Library.
 *
 *
 * @section installation Installation
 *
 * A source RPM and/or binary RPM "lib-zfcp-hbaapi" should be provided which
 * will install the ZFCP HBA API Library called libzfcphbaapi.so and the
 * corresponding header file hbaapi.h.
 *
 * @section logging Logging
 *
 * To log error situations in ZFCP HBA API Library two environment variables
 * are used:
 *
 *	- LIB_ZFCP_HBAAPI_LOG_LEVEL - specifies log level
 *		- if not set or set to 0, logging is disabled (default)
 *		- if set to value > 0, logging is enabled
 *
 *	- LIB_ZFCP_HBAAPI_LOG_FILE - specifies file where log output is
 *	written to
 *		- if not set, stderr is used
 *		- if set, specified file is used for log output
 *
 *
 * @section bibliography Bibliography
 *
 * @subsection FCHBA FC-HBA
 * The T11 Technical Committee. Information Technology - Fibre Channel HBA API
 * (FC-HBA). Working Draft, Revision 14, 2004. URL http://www.t11.org
 *
 */

/** @defgroup SupportedHBAAPIs Supported HBA API Functions */
/** @defgroup UnSupportedHBAAPIs Not Supported HBA API Functions */
/** @defgroup InitAndFini Initialization and Finalization Functions */
/** @defgroup Intro */

#include "config.h"

/** This implies definition of _XOPEN_SOURCE. It is needed for strptime(). */
#define _GNU_SOURCE

#include <execinfo.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <scsi/scsi_netlink_fc.h>
#include <dirent.h>

#include <hbaapi.h>


#ifdef HBAAPI_VENDOR_LIB	/* compile as vendor specific library */
/*
 * prototypes for vendor libraries
 */

/* library control */
typedef HBA_UINT32(*HBAGetVersionFunc)(void);
typedef HBA_STATUS(*HBALoadLibraryFunc)(void);
typedef HBA_STATUS(*HBAFreeLibraryFunc)(void);
typedef HBA_UINT32(*HBAGetVendorLibraryAttributesFunc)(HBA_LIBRARYATTRIBUTES *);
typedef HBA_UINT32(*HBAGetNumberOfAdaptersFunc)(void);
typedef void(*HBARefreshInformationFunc)(HBA_HANDLE);
typedef void(*HBARefreshAdapterConfigurationFunc)(void);
typedef void(*HBAResetStatisticsFunc)(HBA_HANDLE, HBA_UINT32);

/* adapter and port information */
typedef HBA_STATUS(*HBAGetAdapterNameFunc)(HBA_UINT32, char *);
typedef HBA_HANDLE(*HBAOpenAdapterFunc)(char *);
typedef HBA_STATUS(*HBAOpenAdapterByWWNFunc)(HBA_HANDLE *, HBA_WWN);
typedef void(*HBACloseAdapterFunc)(HBA_HANDLE);
typedef HBA_STATUS(*HBAGetAdapterAttributesFunc)
     (HBA_HANDLE, HBA_ADAPTERATTRIBUTES *);
typedef HBA_STATUS(*HBAGetAdapterPortAttributesFunc)
     (HBA_HANDLE, HBA_UINT32, HBA_PORTATTRIBUTES *);
typedef HBA_STATUS(*HBAGetDiscoveredPortAttributesFunc)
     (HBA_HANDLE, HBA_UINT32, HBA_UINT32, HBA_PORTATTRIBUTES *);
typedef HBA_STATUS(*HBAGetPortAttributesByWWNFunc)
     (HBA_HANDLE, HBA_WWN, HBA_PORTATTRIBUTES *);
typedef HBA_STATUS(*HBAGetPortStatisticsFunc)
     (HBA_HANDLE, HBA_UINT32, HBA_PORTSTATISTICS *);
typedef HBA_STATUS(*HBAGetFC4StatisticsFunc)
     (HBA_HANDLE, HBA_WWN, HBA_UINT8, HBA_FC4STATISTICS *);

/* FCP information */
typedef HBA_STATUS(*HBAGetBindingCapabilityFunc)
     (HBA_HANDLE, HBA_WWN, HBA_BIND_CAPABILITY *);
typedef HBA_STATUS(*HBAGetBindingSupportFunc)
     (HBA_HANDLE, HBA_WWN, HBA_BIND_CAPABILITY *);
typedef HBA_STATUS(*HBASetBindingSupportFunc)
     (HBA_HANDLE, HBA_WWN, HBA_BIND_CAPABILITY);
typedef HBA_STATUS(*HBAGetFcpTargetMappingFunc)
     (HBA_HANDLE, HBA_FCPTARGETMAPPING *);
typedef HBA_STATUS(*HBAGetFcpTargetMappingV2Func)
     (HBA_HANDLE, HBA_WWN, HBA_FCPTARGETMAPPINGV2 *);
typedef HBA_STATUS(*HBAGetFcpPersistentBindingFunc)
     (HBA_HANDLE, HBA_FCPBINDING *);
typedef HBA_STATUS(*HBAGetPersistentBindingV2Func)
     (HBA_HANDLE, HBA_WWN, HBA_FCPBINDING2 *);
typedef HBA_STATUS(*HBASetPersistentBindingV2Func)
     (HBA_HANDLE, HBA_WWN, HBA_FCPBINDING2 *);
typedef HBA_STATUS(*HBARemovePersistentBindingFunc)
     (HBA_HANDLE, HBA_WWN, HBA_FCPBINDING2 *);
typedef HBA_STATUS(*HBARemoveAllPersistentBindingsFunc)(HBA_HANDLE, HBA_WWN);
typedef HBA_STATUS(*HBAGetFCPStatisticsFunc)
     (HBA_HANDLE, const HBA_SCSIID *, HBA_FC4STATISTICS *);

/* SCSI information */
typedef HBA_STATUS(*HBASendScsiInquiryFunc)
     (HBA_HANDLE, HBA_WWN, HBA_UINT64, HBA_UINT8, HBA_UINT32, void *,
      HBA_UINT32, void *, HBA_UINT32);
typedef HBA_STATUS(*HBAScsiInquiryV2Func)
     (HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT64, HBA_UINT8, HBA_UINT8, void *,
      HBA_UINT32 *, HBA_UINT8 *, void *, HBA_UINT32 *);
typedef HBA_STATUS(*HBASendReportLUNsFunc)
     (HBA_HANDLE, HBA_WWN, void *, HBA_UINT32, void *, HBA_UINT32);
typedef HBA_STATUS(*HBAScsiReportLUNsV2Func)
     (HBA_HANDLE, HBA_WWN, HBA_WWN, void *, HBA_UINT32 *, HBA_UINT8 *, void *,
      HBA_UINT32 *);
typedef HBA_STATUS(*HBASendReadCapacityFunc)
     (HBA_HANDLE, HBA_WWN, HBA_UINT64, void *, HBA_UINT32, void *, HBA_UINT32);
typedef HBA_STATUS(*HBAScsiReadCapacityV2Func)
     (HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT64, void *, HBA_UINT32 *,
      HBA_UINT8 *, void *, HBA_UINT32 *);

/* fabric management */
typedef HBA_STATUS(*HBASendCTPassThruFunc)
     (HBA_HANDLE, void *, HBA_UINT32, void *, HBA_UINT32);
typedef HBA_STATUS(*HBASendCTPassThruV2Func)
     (HBA_HANDLE, HBA_WWN, void *, HBA_UINT32, void *, HBA_UINT32 *);
typedef HBA_STATUS(*HBASetRNIDMgmtInfoFunc)(HBA_HANDLE, HBA_MGMTINFO *);
typedef HBA_STATUS(*HBAGetRNIDMgmtInfoFunc)(HBA_HANDLE, HBA_MGMTINFO *);
typedef HBA_STATUS(*HBASendRNIDFunc)
     (HBA_HANDLE, HBA_WWN, HBA_WWNTYPE, void *, HBA_UINT32 *);
typedef HBA_STATUS(*HBASendRNIDV2Func)
     (HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT32, HBA_UINT32, void *,
      HBA_UINT32 *);
typedef HBA_STATUS(*HBASendRPLFunc)
     (HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT32, HBA_UINT32, void *,
      HBA_UINT32 *);
typedef HBA_STATUS(*HBASendRPSFunc)
     (HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT32, HBA_WWN, HBA_UINT32, void *,
      HBA_UINT32 *);
typedef HBA_STATUS(*HBASendSRLFunc)
     (HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT32, void *, HBA_UINT32 *);
typedef HBA_STATUS(*HBASendLIRRFunc)
     (HBA_HANDLE, HBA_WWN, HBA_WWN, HBA_UINT8, HBA_UINT8, void *, HBA_UINT32 *);
typedef HBA_STATUS(*HBASendRLSFunc)
     (HBA_HANDLE, HBA_WWN, HBA_WWN, void *, HBA_UINT32 *);

/* event handling */
typedef HBA_STATUS(*HBAGetEventBufferFunc)
     (HBA_HANDLE, HBA_EVENTINFO *, HBA_UINT32 *);
typedef HBA_STATUS(*HBARegisterForAdapterAddEventsFunc)
     (void (*)(void *, HBA_WWN, HBA_UINT32), void *, HBA_CALLBACKHANDLE *);
typedef HBA_STATUS(*HBARegisterForAdapterEventsFunc)
     (void (*)(void *, HBA_WWN, HBA_UINT32), void *, HBA_HANDLE,
      HBA_CALLBACKHANDLE *);
typedef HBA_STATUS(*HBARegisterForAdapterPortEventsFunc)
     (void (*)(void *, HBA_WWN, HBA_UINT32, HBA_UINT32), void *, HBA_HANDLE,
      HBA_WWN, HBA_CALLBACKHANDLE *);
typedef HBA_STATUS(*HBARegisterForAdapterPortStatEventsFunc)
     (void (*)(void *, HBA_WWN, HBA_UINT32), void *, HBA_HANDLE, HBA_WWN,
      HBA_PORTSTATISTICS, HBA_UINT32, HBA_CALLBACKHANDLE *);
typedef HBA_STATUS(*HBARegisterForTargetEventsFunc)
     (void (*)(void *, HBA_WWN, HBA_WWN, HBA_UINT32), void *, HBA_HANDLE,
      HBA_WWN, HBA_WWN, HBA_CALLBACKHANDLE *, HBA_UINT32);
typedef HBA_STATUS(*HBARegisterForLinkEventsFunc)
     (void (*)(void *, HBA_WWN, HBA_UINT32, void *, HBA_UINT32), void *,
      void *, HBA_UINT32, HBA_HANDLE, HBA_CALLBACKHANDLE *);
typedef HBA_STATUS(*HBARemoveCallbackFunc)(HBA_CALLBACKHANDLE);

/* libraries with support for HBA API Phase 1 and 2 must provide both structs,
   HBA_EntryPoints and HBA_EntryPoints2 */
typedef struct HBA_EntryPoints {
	HBAGetVersionFunc GetVersionHandler;
	HBALoadLibraryFunc LoadLibraryHandler;
	HBAFreeLibraryFunc FreeLibraryHandler;
	HBAGetNumberOfAdaptersFunc GetNumberOfAdaptersHandler;
	HBAGetAdapterNameFunc GetAdapterNameHandler;
	HBAOpenAdapterFunc OpenAdapterHandler;
	HBACloseAdapterFunc CloseAdapterHandler;
	HBAGetAdapterAttributesFunc GetAdapterAttributesHandler;
	HBAGetAdapterPortAttributesFunc GetAdapterPortAttributesHandler;
	HBAGetPortStatisticsFunc GetPortStatisticsHandler;
	 HBAGetDiscoveredPortAttributesFunc
	    GetDiscoveredPortAttributesHandler;
	HBAGetPortAttributesByWWNFunc GetPortAttributesByWWNHandler;
	HBASendCTPassThruFunc SendCTPassThruHandler;
	HBARefreshInformationFunc RefreshInformationHandler;
	HBAResetStatisticsFunc ResetStatisticsHandler;
	HBAGetFcpTargetMappingFunc GetFcpTargetMappingHandler;
	HBAGetFcpPersistentBindingFunc GetFcpPersistentBindingHandler;
	HBAGetEventBufferFunc GetEventBufferHandler;
	HBASetRNIDMgmtInfoFunc SetRNIDMgmtInfoHandler;
	HBAGetRNIDMgmtInfoFunc GetRNIDMgmtInfoHandler;
	HBASendRNIDFunc SendRNIDHandler;
	HBASendScsiInquiryFunc ScsiInquiryHandler;
	HBASendReportLUNsFunc ReportLUNsHandler;
	HBASendReadCapacityFunc ReadCapacityHandler;
} HBA_ENTRYPOINTS;

HBA_STATUS HBA_RegisterLibrary(HBA_ENTRYPOINTS *);

typedef struct HBA_EntryPointsV2 {
	HBAGetVersionFunc GetVersionHandler;
	HBALoadLibraryFunc LoadLibraryHandler;
	HBAFreeLibraryFunc FreeLibraryHandler;
	HBAGetNumberOfAdaptersFunc GetNumberOfAdaptersHandler;
	HBAGetAdapterNameFunc GetAdapterNameHandler;
	HBAOpenAdapterFunc OpenAdapterHandler;
	HBACloseAdapterFunc CloseAdapterHandler;
	HBAGetAdapterAttributesFunc GetAdapterAttributesHandler;
	HBAGetAdapterPortAttributesFunc GetAdapterPortAttributesHandler;
	HBAGetPortStatisticsFunc GetPortStatisticsHandler;
	 HBAGetDiscoveredPortAttributesFunc
	    GetDiscoveredPortAttributesHandler;
	HBAGetPortAttributesByWWNFunc GetPortAttributesByWWNHandler;
	HBASendCTPassThruFunc SendCTPassThruHandler;
	HBARefreshInformationFunc RefreshInformationHandler;
	HBAResetStatisticsFunc ResetStatisticsHandler;
	HBAGetFcpTargetMappingFunc GetFcpTargetMappingHandler;
	HBAGetFcpPersistentBindingFunc GetFcpPersistentBindingHandler;
	HBAGetEventBufferFunc GetEventBufferHandler;
	HBASetRNIDMgmtInfoFunc SetRNIDMgmtInfoHandler;
	HBAGetRNIDMgmtInfoFunc GetRNIDMgmtInfoHandler;
	HBASendRNIDFunc SendRNIDHandler;
	HBASendScsiInquiryFunc ScsiInquiryHandler;
	HBASendReportLUNsFunc ReportLUNsHandler;
	HBASendReadCapacityFunc ReadCapacityHandler;
	HBAOpenAdapterByWWNFunc OpenAdapterByWWNHandler;
	HBAGetFcpTargetMappingV2Func GetFcpTargetMappingV2Handler;
	HBASendCTPassThruV2Func SendCTPassThruV2Handler;
	HBARefreshAdapterConfigurationFunc RefreshAdapterConfigurationHandler;
	HBAGetBindingCapabilityFunc GetBindingCapabilityHandler;
	HBAGetBindingSupportFunc GetBindingSupportHandler;
	HBASetBindingSupportFunc SetBindingSupportHandler;
	HBASetPersistentBindingV2Func SetPersistentBindingV2Handler;
	HBAGetPersistentBindingV2Func GetPersistentBindingV2Handler;
	HBARemovePersistentBindingFunc RemovePersistentBindingHandler;
	 HBARemoveAllPersistentBindingsFunc
	    RemoveAllPersistentBindingsHandler;
	HBASendRNIDV2Func SendRNIDV2Handler;
	HBAScsiInquiryV2Func ScsiInquiryV2Handler;
	HBAScsiReportLUNsV2Func ScsiReportLUNsV2Handler;
	HBAScsiReadCapacityV2Func ScsiReadCapacityV2Handler;
	 HBAGetVendorLibraryAttributesFunc
	    GetVendorLibraryAttributesHandler;
	HBARemoveCallbackFunc RemoveCallbackHandler;
	 HBARegisterForAdapterAddEventsFunc
	    RegisterForAdapterAddEventsHandler;
	HBARegisterForAdapterEventsFunc RegisterForAdapterEventsHandler;
	 HBARegisterForAdapterPortEventsFunc
	    RegisterForAdapterPortEventsHandler;
	 HBARegisterForAdapterPortStatEventsFunc
	    RegisterForAdapterPortStatEventsHandler;
	HBARegisterForTargetEventsFunc RegisterForTargetEventsHandler;
	HBARegisterForLinkEventsFunc RegisterForLinkEventsHandler;
	HBASendRPLFunc SendRPLHandler;
	HBASendRPSFunc SendRPSHandler;
	HBASendSRLFunc SendSRLHandler;
	HBASendLIRRFunc SendLIRRHandler;
	HBAGetFC4StatisticsFunc GetFC4StatisticsHandler;
	HBAGetFCPStatisticsFunc GetFCPStatisticsHandler;
	HBASendRLSFunc SendRLSHandler;
} HBA_ENTRYPOINTSV2;

HBA_STATUS HBA_RegisterLibraryV2(HBA_ENTRYPOINTSV2 *);

#endif /* HBAAPI_VENDOR_LIB */

/** @brief This is a phase 2 HBA API library. */
#define HBAAPI_LIBRARY_VERSION 2

/** @brief Define as 1 if this library implements the final FC-HBA,
    which is not yet the case. */
#define HBAAPI_LIBRARY_FINAL 0

/** @brief Revision of this library. */
#define HBAAPI_LIBRARY_REVISION VERSION

/** @brief Environment variable to enable logging */
#define VLIB_ENV_LOG_LEVEL	"LIB_ZFCP_HBAAPI_LOG_LEVEL"

/** @brief Environment variable specifying the file which is used for logging */
#define VLIB_ENV_LOG_FILE	"LIB_ZFCP_HBAAPI_LOG_FILE"

/** @brief Prefix used to concatednate an adapter name. */
#define VLIB_ADAPTERNAME_PREFIX "com.ibm-FICON-FCP-"

/** @brief Maximal lenght of an adapter name as used in this libary. */
#define VLIB_ADAPTERNAME_LEN	256

/** @brief This is the value of an invalid handle as used in this library. */
#define VLIB_INVALID_HANDLE	0

/** @brief This is the value the report luns well known lun */
#define REPORTLUNS_WLUN 0xc101000000000000
#define REPORTLUNS_WLUN_DEC 49409

typedef uint64_t devid_t;
typedef uint64_t wwn_t;
typedef uint32_t fc_id_t;
typedef uint64_t fcp_lun_t;

/** @brief Event data structure used in the library. */
struct vlib_event {
	HBA_EVENTINFO hbaapi_event;
	struct vlib_event *next;
};

/** @brief Event queue data structure used in the library. */
struct vlib_event_queue {
	unsigned int slots_used;
	struct vlib_event *first;
	struct vlib_event *last;
};

/** @brief Block structure used to hold all needed data for growable arrays. */
struct block {
	void *data;	/**< @brief pointer to an array */
	size_t used;	/**< @brief number of used elements in the array */
	size_t allocated; /**< @brief total number of elements in the array */
};

/** @brief Represenation of an FCP unit in the library. */
struct vlib_unit {
	unsigned int isInvalid:1;	/**< @brief Unit invalid or not */
	unsigned int host;		/**< @brief SCSI host */
	unsigned int channel;		/**< @brief SCSI channel */
	unsigned int target;		/**< @brief SCSI id */
	unsigned int lun;		/**< @brief SCSI LUN */
	uint64_t fcLun;			/**< @brief FCP LUN */
	char sg_dev[16];		/**< @brief name of sg device */
};

/** @brief Representation of a FC port in the library */
struct vlib_port {
	unsigned int isInvalid:1;	/**< @brief Port invalid or not */
	wwn_t wwpn;			/**< @brief WWPN of the port */
	wwn_t wwnn;			/**< @brief WWNN of the port */
	fc_id_t did;			/**< @brief FC did of the port */
	struct block units;		/**< @brief List of units */
	char name[32];			/**< @brief name as in sysfs
						under fc_remote_ports */
	unsigned int host;		/**< @brief SCSI host */
	unsigned int channel;		/**< @brief SCSI channel */
	unsigned int target;		/**< @brief SCSI id */
};

/** @brief Identification of an adapter in the library */
struct vlib_adapter_ident {
	devid_t devid;		/**< @brief Unique id for adapter device */
	wwn_t wwnn;		/**< @brief WWN of adapter node */
	wwn_t wwpn;		/**< @brief WWN of adapter port */
	unsigned short host;	/**< @brief SCSI host id of this adapter */
	fc_id_t did;		/**< @brief FC did of the port */
	char bus_dev_name[9];	/**< @brief name of device as in
					/sys/bus/ccw/drivers/zfcp
					in the form "x.x.xxxx" */
	char class_dev_name[9];	/**< @brief name of device as in
					/sys/class/fc_host in the form
					"hostxxxx" */
	char sysfsPath[PATH_MAX]; /**< @brief path of adapter in sysfs in the
				form  /sys/devices/css0/0.0.0010/0.0.5923*/
};

/** @brief Represenation of an adapter in the library */
struct vlib_adapter {
	unsigned int isInvalid:1;	/**< @brief Adapter invalid or not */
	struct vlib_adapter_ident ident; /**< @brief Adapter identification */
	HBA_HANDLE handle;		/**< @brief Handle for this adapter */
	struct block ports;		/**< @brief List of ports */
	struct vlib_event_queue event_queue;     /**< @brief Event queue */
	struct vlib_event_queue free_event_list; /**< @brief Free slots */
};

/** @brief Primary data structure used in the library. */
struct vlib_data {
	unsigned int isLoaded:1;	/**< @brief Library loaded or not */
	unsigned int unloading:1;	/**< @brief Library unloaded */
	unsigned int isValid:1;		/**< @brief Repositoy valid or not
					   This flag is set for instance if a
					   loss of events is detected. */
	int loglevel;			/**< @brief loglevel for library
					   Default is 0 -- no logging. */
	FILE *errfp;			/**< @brief file used for logging
					   Default is stderr. */
	struct block adapters;		/**< @brief List of adapters
					   In fact this is the anchor of
					   the library's repository. */
	pthread_t id;			/**< @brief Pthread ID of event
					   handling thread*/
	pthread_mutex_t mutex;		/**< @brief Protects this structure */
};

/**
 * @brief Global variable that holds almost all information needed in
 *	the library.
 *
 * All data except some data needed for %event handling is stored in this
 * variable. To be thread safe, access to this variable must be locked using
 * vlib_data.mutex. vlib_data.mutex is initialized in the initialization
 * function and is destroyed in the finalization function of the library.
 */
extern struct vlib_data vlib_data;

/** @brief This is a wrapper for the usage of strerror_r(). */
static inline void vlib_print_error(int errnum, const char *function,
				    const char *file, int line,
				    char *fmt, ...)
{
	char buffer[256];
	va_list ap;

	if (0 == vlib_data.loglevel)
		return;

	fprintf(vlib_data.errfp, "(%s:%d)%s: ", file, line, function);

	va_start(ap, fmt);
	vfprintf(vlib_data.errfp, fmt, ap);
	va_end(ap);

	if (errnum != 0) {
		fprintf(vlib_data.errfp, ": %s\n",
			strerror_r(errnum, buffer, 256));
	}
}

/** @brief To log errors in the library, this macro is used which invokes
    vlib_print_error(). */
#define VLIB_PERROR(errnum, fmt, args...) \
vlib_print_error(errnum, __func__, __FILE__, __LINE__, fmt, ##args)

/** @brief To log messages in the library, this macro is used. */
#define VLIB_LOG(fmt, args...) \
vlib_print_error(0, __func__, __FILE__, __LINE__, fmt, ##args)

/**
 * @brief Print a stack backtrace.
 * @note This needs -rdynamic to work.
 * @note This function is only defined if HBAAPI_BACKTRACE is defined.
 */
#ifdef HBAAPI_BACKTRACE
#define BT_SIZE 50
static inline void vlib_bt(void)
{
	int num;
	void *array[BT_SIZE] = { 0 };

	num = backtrace(array, BT_SIZE);
	backtrace_symbols_fd(array, num, fileno(stderr));
}
#else
#define vlib_bt()
#endif

/**
 * @brief To lock a mutex, this macro is used, which checks for errors.
 *
 * @note The programm is exited if:
 *	- mutex is not properly initialized or
 *	- a deadlock situation is encountered and HBAAPI_BACKTRACE is defined.
 */
#define VLIB_MUTEX_LOCK(mutex) \
do { \
	int __ret; \
	__ret = pthread_mutex_lock(mutex); \
	if (__ret == EDEADLK) { \
		VLIB_LOG("BUG: VLIB_MUTEX_LOCK: deadlock detected!\n"); \
		vlib_bt(); \
		exit(1); \
	} else if (__ret == EINVAL) { \
		VLIB_LOG("BUG: VLIB_MUTEX_LOCK: mutex not properly " \
			 "initialized\n"); \
		vlib_bt(); \
		exit(1); \
	} \
} while (0)

/**
 * @brief To unlock a mutex, this macro is used, which checks for errors.
 *
 * @note The programm is exited if:
 *	- mutex is not properly initialized or
 *	- mutex is not own by this thread and HBAAPI_BACKTRACE is defined.
 */
#define VLIB_MUTEX_UNLOCK(mutex) \
do { \
	int __ret; \
	__ret = pthread_mutex_unlock(mutex); \
	if (__ret == EPERM) { \
		VLIB_LOG("BUG: VLIB_MUTEX_UNLOCK: thread does not own " \
			 "mutex!\n"); \
		vlib_bt(); \
		exit(1); \
	} else if (__ret == EINVAL) { \
		VLIB_LOG("BUG: VLIB_MUTEX_UNLOCK: " \
			 "mutex not properly initialized\n"); \
		vlib_bt(); \
		exit(1); \
	} \
} while (0)

/* Include auxiliary stuff, like inline and helper functions etc. */
#include "vlib_aux.h"
#include "vlib_sysfs.h"
#include "vlib_sg.h"
#include "vlib_sg_io.h"
#include "vlib_events.h"
#include "vlib_sfhelper.h"

#endif /* _VLIB_H_ */
