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
 * File:		vlib.c
 *
 * Description:
 * This file contains the implementations of most HBA API library
 * functions.
 *
 */

/**
 * @file vlib.c
 * @brief Implementations of most HBA API functions (all except event handling
 *	functions).
 * @note We only include vlib.h which in turn should include all other
 *	necessary header files.
 *
 * This file contains the implementations of most HBA API library
 * functions. Implementations of event handling HBA API routines
 * can be found in files vlib_events.c and vlib_callbacks.c.
 * @see vlib_events.c vlib_callbacks.c
 */

#include "vlib.h"

/*
 *	GLOBAL VARIABLES
 */
struct vlib_data vlib_data;

/*
 *	STATIC FUNCTION DECLARATIONS
 */

/**
 * @brief Internal function to report the attributes of the libary.
 */
static HBA_UINT32 _GetVendorLibraryAttributes(HBA_LIBRARYATTRIBUTES *);

/*
 *	LIBRARY CONTROL FUNCTIONS
 */

/** @ingroup SupportedHBAAPIs
 * @brief Return the version of the HBA API specification with which this
 *	library is compliant.
 * @return
 * 	- 2 (compliant to FC-HBA)
 * @note No check if library is loaded has to be performed.
 */
HBA_UINT32 HBA_GetVersion(void)
{
	return HBAAPI_LIBRARY_VERSION;
}


/** @ingroup InitAndFini
 * @brief Initialization function of this library.
 */
void _initvlib(void)
{
	char *env;

	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);

#ifdef HBAAPI_BACKTRACE
	pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_ERRORCHECK_NP);
#endif

	memset(&vlib_data, 0, sizeof(vlib_data));

	env = getenv(VLIB_ENV_LOG_LEVEL);
	if (env != NULL)
		vlib_data.loglevel = atoi(env);

	vlib_data.errfp = stderr;
	env = getenv(VLIB_ENV_LOG_FILE);
	if (env != NULL) {
		vlib_data.errfp = fopen(env, "a");
		if (NULL == vlib_data.errfp) {
			vlib_data.errfp = stderr;
			VLIB_PERROR(errno, "WARNING: fopen() failed for log "
				    "file '%s'", env);
		}
	}

	/* start logging */
	if (vlib_data.loglevel > 0) {
		char timestr[32];
		time_t t;

		t = time(NULL);
		memset(&timestr, 0, 32);
		strftime(timestr, 32, "%b %d %T", localtime(&t));
		VLIB_LOG("libzfcphbaapi.so loaded at %s\n", timestr);
	}

	pthread_mutex_init(&vlib_data.mutex, &mutexattr);
}

/** @ingroup InitAndFini
 * @brief Finalization function of this library.
 */
void _finivlib(void)
{
	if (vlib_data.errfp != stderr)
		fclose(vlib_data.errfp);

	pthread_mutex_destroy(&vlib_data.mutex);
}

/** @ingroup SupportedHBAAPIs
 * @brief Perform initialization of library.
 * @return
 * 	- HBA_STATUS_ERROR_ALREADY_LOADED if this function was already called
 * 	  before.
 * 	- HBA_STATUS_ERROR if initialization fails.
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note This function tries to open the zfcp_hbaapi char device.
 */
HBA_STATUS HBA_LoadLibrary(void)
{
	HBA_STATUS status;
	int ret;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	if (vlib_data.isLoaded) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR;
	}

	status = sysfs_createAndReadConfigAdapter();
	if (status != HBA_STATUS_OK) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	vlib_data.isLoaded = 1;

	start_event_thread();

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	return status;
}

/** @ingroup SupportedHBAAPIs
 * @brief Free system resources that library has used.
 * @return
 * 	- HBA_STATUS_ERROR_NOT_LOADED if HBA_LoadLibrary was not called before.
 *	- HBA_STATUS_ERROR if HBA_FreeLibrary is already running.
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note This function tries to close the zfcp_hbaapi char device.
 */
HBA_STATUS HBA_FreeLibrary(void)
{
	void *res;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);
	if (!vlib_data.isLoaded) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR;
	}


	if (vlib_data.unloading) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR;
	}
	vlib_data.unloading = 1;

	if (pthread_cancel(vlib_data.id) != 0) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR;
	}

	if (pthread_join(vlib_data.id, &res) != 0) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR;
	}

	if (res != PTHREAD_CANCELED) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR;
	}

	closeAllAdapters();

	vlib_data.isLoaded = 0;
	vlib_data.unloading = 0;
	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	return HBA_STATUS_OK;
}

#ifdef HBAAPI_VENDOR_LIB	/* compile as vendor specific library */

/** @ingroup SupportedHBAAPIs
 * @brief Register the functionality of FC-MI with the wrapper library.
 * @param pHBAInfo entry points of HBA API phase 1 library functions
 * @return HBA_STATUS_OK
 * @note This function is only defined if this library was built as a
 *	vendor specific library.
 */
HBA_STATUS HBA_RegisterLibrary(HBA_ENTRYPOINTS *pHBAInfo)
{
	memset(pHBAInfo, 0, sizeof(*pHBAInfo));

	pHBAInfo->GetVersionHandler = HBA_GetVersion;
	pHBAInfo->LoadLibraryHandler = HBA_LoadLibrary;
	pHBAInfo->FreeLibraryHandler = HBA_FreeLibrary;
	pHBAInfo->GetNumberOfAdaptersHandler = HBA_GetNumberOfAdapters;
	pHBAInfo->GetAdapterNameHandler = HBA_GetAdapterName;
	pHBAInfo->OpenAdapterHandler = HBA_OpenAdapter;
	pHBAInfo->CloseAdapterHandler = HBA_CloseAdapter;
	pHBAInfo->GetAdapterAttributesHandler = HBA_GetAdapterAttributes;
	pHBAInfo->GetAdapterPortAttributesHandler =
		HBA_GetAdapterPortAttributes;
	pHBAInfo->GetPortStatisticsHandler = HBA_GetPortStatistics;
	pHBAInfo->GetDiscoveredPortAttributesHandler =
	    HBA_GetDiscoveredPortAttributes;
	pHBAInfo->GetPortAttributesByWWNHandler = HBA_GetPortAttributesByWWN;
	pHBAInfo->SendCTPassThruHandler = HBA_SendCTPassThru;
	pHBAInfo->RefreshInformationHandler = HBA_RefreshInformation;
	pHBAInfo->ResetStatisticsHandler = HBA_ResetStatistics;
	pHBAInfo->GetFcpTargetMappingHandler = HBA_GetFcpTargetMapping;
	pHBAInfo->GetFcpPersistentBindingHandler = HBA_GetFcpPersistentBinding;
	pHBAInfo->GetEventBufferHandler = HBA_GetEventBuffer;
	pHBAInfo->SetRNIDMgmtInfoHandler = HBA_SetRNIDMgmtInfo;
	pHBAInfo->GetRNIDMgmtInfoHandler = HBA_GetRNIDMgmtInfo;
	pHBAInfo->SendRNIDHandler = HBA_SendRNID;
	pHBAInfo->ScsiInquiryHandler = HBA_SendScsiInquiry;
	pHBAInfo->ReportLUNsHandler = HBA_SendReportLUNs;
	pHBAInfo->ReadCapacityHandler = HBA_SendReadCapacity;

	return HBA_STATUS_OK;
}

/** @ingroup SupportedHBAAPIs
 * @brief Register the functionality of HBA API phase 2 with the wrapper
 *	library.
 * @param pHBAInfo entry points of HBA API phase 2 library functions
 * @return HBA_STATUS_OK on success
 * @note This function is only defined if this library was built as a
 *	vendor specific library.
 */
HBA_STATUS HBA_RegisterLibraryV2(HBA_ENTRYPOINTSV2 *pHBAInfo)
{
	memset(pHBAInfo, 0, sizeof(*pHBAInfo));

	pHBAInfo->GetVersionHandler = HBA_GetVersion;
	pHBAInfo->LoadLibraryHandler = HBA_LoadLibrary;
	pHBAInfo->FreeLibraryHandler = HBA_FreeLibrary;
	pHBAInfo->GetNumberOfAdaptersHandler = HBA_GetNumberOfAdapters;
	pHBAInfo->GetAdapterNameHandler = HBA_GetAdapterName;
	pHBAInfo->OpenAdapterHandler = HBA_OpenAdapter;
	pHBAInfo->CloseAdapterHandler = HBA_CloseAdapter;
	pHBAInfo->GetAdapterAttributesHandler = HBA_GetAdapterAttributes;
	pHBAInfo->GetAdapterPortAttributesHandler =
		HBA_GetAdapterPortAttributes;
	pHBAInfo->GetPortStatisticsHandler = HBA_GetPortStatistics;
	pHBAInfo->GetDiscoveredPortAttributesHandler =
		HBA_GetDiscoveredPortAttributes;
	pHBAInfo->GetPortAttributesByWWNHandler = HBA_GetPortAttributesByWWN;
	pHBAInfo->SendCTPassThruHandler = HBA_SendCTPassThru;
	pHBAInfo->RefreshInformationHandler = HBA_RefreshInformation;
	pHBAInfo->ResetStatisticsHandler = HBA_ResetStatistics;
	pHBAInfo->GetFcpTargetMappingHandler = HBA_GetFcpTargetMapping;
	pHBAInfo->GetFcpPersistentBindingHandler = HBA_GetFcpPersistentBinding;
	pHBAInfo->GetEventBufferHandler = HBA_GetEventBuffer;
	pHBAInfo->SetRNIDMgmtInfoHandler = HBA_SetRNIDMgmtInfo;
	pHBAInfo->GetRNIDMgmtInfoHandler = HBA_GetRNIDMgmtInfo;
	pHBAInfo->SendRNIDHandler = HBA_SendRNID;
	pHBAInfo->ScsiInquiryHandler = HBA_SendScsiInquiry;
	pHBAInfo->ReportLUNsHandler = HBA_SendReportLUNs;
	pHBAInfo->ReadCapacityHandler = HBA_SendReadCapacity;
	pHBAInfo->OpenAdapterByWWNHandler = HBA_OpenAdapterByWWN;
	pHBAInfo->GetFcpTargetMappingV2Handler = HBA_GetFcpTargetMappingV2;
	pHBAInfo->SendCTPassThruV2Handler = HBA_SendCTPassThruV2;
	pHBAInfo->RefreshAdapterConfigurationHandler =
	    HBA_RefreshAdapterConfiguration;
	pHBAInfo->GetBindingCapabilityHandler = HBA_GetBindingCapability;
	pHBAInfo->GetBindingSupportHandler = HBA_GetBindingSupport;
	pHBAInfo->SetBindingSupportHandler = HBA_SetBindingSupport;
	pHBAInfo->SetPersistentBindingV2Handler = HBA_SetPersistentBindingV2;
	pHBAInfo->GetPersistentBindingV2Handler = HBA_GetPersistentBindingV2;
	pHBAInfo->RemovePersistentBindingHandler = HBA_RemovePersistentBinding;
	pHBAInfo->RemoveAllPersistentBindingsHandler =
	    HBA_RemoveAllPersistentBindings;
	pHBAInfo->SendRNIDV2Handler = HBA_SendRNIDV2;
	pHBAInfo->ScsiInquiryV2Handler = HBA_ScsiInquiryV2;
	pHBAInfo->ScsiReportLUNsV2Handler = HBA_ScsiReportLUNsV2;
	pHBAInfo->ScsiReadCapacityV2Handler = HBA_ScsiReadCapacityV2;
	pHBAInfo->GetVendorLibraryAttributesHandler =
	    _GetVendorLibraryAttributes;
	pHBAInfo->RemoveCallbackHandler = HBA_RemoveCallback;
	pHBAInfo->RegisterForAdapterAddEventsHandler =
	    HBA_RegisterForAdapterAddEvents;
	pHBAInfo->RegisterForAdapterEventsHandler =
		HBA_RegisterForAdapterEvents;
	pHBAInfo->RegisterForAdapterPortEventsHandler =
	    HBA_RegisterForAdapterPortEvents;
	pHBAInfo->RegisterForAdapterPortStatEventsHandler =
	    HBA_RegisterForAdapterPortStatEvents;
	pHBAInfo->RegisterForTargetEventsHandler = HBA_RegisterForTargetEvents;
	pHBAInfo->RegisterForLinkEventsHandler = HBA_RegisterForLinkEvents;
	pHBAInfo->SendRPLHandler = HBA_SendRPL;
	pHBAInfo->SendRPSHandler = HBA_SendRPS;
	pHBAInfo->SendSRLHandler = HBA_SendSRL;
	pHBAInfo->SendLIRRHandler = HBA_SendLIRR;
	pHBAInfo->GetFC4StatisticsHandler = HBA_GetFC4Statistics;
	pHBAInfo->GetFCPStatisticsHandler = HBA_GetFCPStatistics;
	pHBAInfo->SendRLSHandler = HBA_SendRLS;

	return HBA_STATUS_OK;
}

#endif /* HBAAPI_VENDOR_LIB */

#ifndef HBAAPI_VENDOR_LIB
/** @ingroup SupportedHBAAPIs
 * @brief Return attributes of the OS specific HBA API library.
 * @param attributes used to return library attributes
 * @return
 * 	- 2 (compliant to FC-HBA)
 * @note This function is not defined if the library is built as a
 *	vendor specific library.
 * @see _GetVendorLibraryAttributes()
 */
HBA_UINT32 HBA_GetWrapperLibraryAttributes(HBA_LIBRARYATTRIBUTES *attributes)
{
	return _GetVendorLibraryAttributes(attributes);
}

/** @ingroup SupportedHBAAPIs
 * @brief Return attributes of the vendor specific HBA API library.
 * @param adapter_index not used
 * @param attributes used to return library attributes
 * @return
 * 	- 2 (compliant to FC-HBA)
 * @note This function is not defined if the library is built as a
 *	vendor specific library.
 * @note The index of the HBA is ignored, because it is of use only for a
 *	wrapper library.
 * @see _GetVendorLibraryAttributes()
 */
HBA_UINT32 HBA_GetVendorLibraryAttributes(HBA_UINT32 adapter_index,
					  HBA_LIBRARYATTRIBUTES *attributes)
{
	return _GetVendorLibraryAttributes(attributes);
}
#endif

/**
 * @param attributes used to return library attributes
 * @return
 * 	- 2 (compliant to FC-HBA)
 */
static HBA_UINT32 _GetVendorLibraryAttributes(HBA_LIBRARYATTRIBUTES *attributes)
{
	memset(attributes, 0, sizeof(*attributes));
	attributes->final = HBAAPI_LIBRARY_FINAL;
#ifndef VLIBPATH
# error  "VLIBPATH not set"
#endif
	strncpy(attributes->LibPath, VLIBPATH, 256);
	strncpy(attributes->VName, "IBM Corp.", 256);
	strncpy(attributes->VVersion, HBAAPI_LIBRARY_REVISION, 256);
	strptime(__DATE__ " " __TIME__ , "%b %d %Y %T",
		 &attributes->build_date);

	return HBAAPI_LIBRARY_VERSION;
}

/** @ingroup SupportedHBAAPIs
 * @brief Return number of adapters.
 * @return
 * 	- 0 on error or if no adapters are configured
 * 	- number of adapters on success
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 */
HBA_UINT32 HBA_GetNumberOfAdapters(void)
{
	HBA_UINT32 num;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	if (HBA_STATUS_OK != revalidateRepository())
		num = 0;
	else
		num = vlib_data.adapters.used;

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	return num;
}

/** @ingroup SupportedHBAAPIs
 * @brief Refresh information of an adapter.
 * @param handle of the adapter for which information should be refreshed.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 */
void HBA_RefreshInformation(HBA_HANDLE handle)
{
	HBA_STATUS status;
	struct vlib_adapter *adapter;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_LOG("ERROR: invalid adapter handle or "
			 "adapter unavailable\n");
	} else {
		updateAdapter(adapter);
	}

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	return;
}

#ifdef HBAAPI_VENDOR_LIB	/* compile as vendor specific library */

/** @ingroup SupportedHBAAPIs
 * @brief According to a discussion with Bob Nixon (editor of FC-HBA) this
 *	function has no effect in an HBA specific library.
 */
void HBA_RefreshAdapterConfiguration(void)
{
	return;
}

#else

/** @ingroup SupportedHBAAPIs
 * @brief Refresh information about configured adapters.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note  We do not report HBA_STATUS_ERROR_STALE_DATA, because we use
 *	semistatic tables internally. We just make use of
 *	HBA_STATUS_ERROR_UNAVAILABLE (e.g. if an adapter is removed).
 */
void HBA_RefreshAdapterConfiguration(void)
{
	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	revalidateRepository();

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
}

#endif

/** @ingroup SupportedHBAAPIs
 * @brief According to FC-HBA this function is obsolete.
 *
 * This function has no effect.
 */
void HBA_ResetStatistics(HBA_HANDLE handle, HBA_UINT32 portindex)
{
	return;
}


/*
 *	ADAPTER and PORT INFORMATION FUNCTIONS
 */


/** @ingroup SupportedHBAAPIs
 * @brief Return name that identifies an adapter.
 * @param adapterindex index of the HBA
 * @param pAdaptername used to return the ASCII string
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_ILLEGAL_INDEX if index is invalid
 *	- HBA_STATUS_ERROR if any other internal error occurs
 *	- HBA_STATUS_OK on success
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @see revalidateRepository()
 */
HBA_STATUS HBA_GetAdapterName(HBA_UINT32 adapterindex, char *pAdaptername)
{
	HBA_STATUS status;
	struct vlib_adapter *adapter;

	*pAdaptername = '\0';

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	status = revalidateRepository();
	if (HBA_STATUS_OK != status) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	adapter = getAdapterByIndex(adapterindex);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_ILLEGAL_INDEX;
	}

	if (adapter->isInvalid)	{
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_UNAVAILABLE;
	}

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	snprintf(pAdaptername, VLIB_ADAPTERNAME_LEN, "%s%u",
		 VLIB_ADAPTERNAME_PREFIX, adapterindex);

	return HBA_STATUS_OK;
}

/** @ingroup SupportedHBAAPIs
 * @brief Open an adapter.
 * @param pAdaptername name of adapter to be opened (name was obtained by
 *	previous call to HBA_GetAdapterName())
 * @return
 * 	- ::VLIB_INVALID_HANDLE on error
 * 	- handle on success
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note Possible (theoretical) overflow of index values.
 */
HBA_HANDLE HBA_OpenAdapter(char *pAdaptername)
{
	HBA_STATUS status;
	HBA_HANDLE handle;
	int index;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	status = revalidateRepository();
	if (HBA_STATUS_OK != status) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return VLIB_INVALID_HANDLE;
	}

	index = findIndexByName(pAdaptername);
	if (-1 == index) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return VLIB_INVALID_HANDLE;
	}

	handle = openAdapterByIndex(index);

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	return handle;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_OpenAdapterByWWN(HBA_HANDLE *pHandle, HBA_WWN wwn)
{
	/* note: wwn is either node name or port name of adapter */
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup SupportedHBAAPIs
 * @brief Close an open adapter.
 * @param handle of adapter to be closed
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 *
 * The adapter handle is invalidated and the information about the
 * ports and units of this adapter is deleted.
 */
void HBA_CloseAdapter(HBA_HANDLE handle)
{
	HBA_STATUS status;
	struct vlib_adapter *adapter;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return;
	}

	doCloseAdapter(adapter);

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	return;
}

/** @ingroup SupportedHBAAPIs
 * @brief Return attributes for an adapter.
 * @param handle of an opened adapter
 * @param pAdapterattributes pointer to return atributes
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note ZFCP HBA API does not set the adapter attributes OptionROMVersion
 *	and NodeSymbolicName.
 */
HBA_STATUS HBA_GetAdapterAttributes(HBA_HANDLE handle,
				    HBA_ADAPTERATTRIBUTES *pAdapterattributes)
{
	HBA_STATUS status;
	int ret;
	struct vlib_adapter *adapter;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	status = revalidateRepository();
	if (HBA_STATUS_OK != status) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	status = sysfs_getAdapterAttributes(&pAdapterattributes, adapter);

	return status;
}

/** @ingroup SupportedHBAAPIs
 * @brief Return attributes for an adapter port.
 * @param handle to an opened adapter
 * @param portindex index of adapter port
 * @param pPortattributes pointer to return atributes
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 * 	- HBA_STATUS_ERROR_ILLEGAL_INDEX if portindex is invalid
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note Parameter portindex _must_ be 0, since we have only one local port on
 *	our adapters.
 * @note Additionally this function triggers creation of port configuration for
 *	this adapter (see revalidatePorts()).
 * @note ZFCP HBA API does not set the port attributes FabricName,
 *	OSDeviceName and PortSymbolicName for an adapter port.
 */
HBA_STATUS HBA_GetAdapterPortAttributes(HBA_HANDLE handle,
					HBA_UINT32 portindex,
					HBA_PORTATTRIBUTES *pPortattributes)
{
	HBA_STATUS status;
	struct vlib_adapter *adapter;
	int ret;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	status = revalidateRepository();
	if (HBA_STATUS_OK != status) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	if (portindex != 0) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_ILLEGAL_INDEX;
	}

	if (revalidatePorts(adapter) < 0) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR;
	}

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	status = sysfs_getAdapterPortAttributes(&pPortattributes, adapter);
	return status;
}

/** @ingroup SupportedHBAAPIs
 * @brief Return attributes of an discovered port.
 * @param handle to an opened adapter
 * @param portindex index of adapter port
 * @param discoveredportindex index of adapter port
 * @param pPortattributes pointer to return atributes
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 * 	- HBA_STATUS_ERROR_ILLEGAL_INDEX if portindex or discoveredindex
 *	 is invalid
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note Parameter portindex _must_ be 0, since we have only one local port on
 *	our adapters.
 * @note For discovered ports ZFCP HBA API does not set the port attributes
 *	OSDeviceName, PortMaxFrameSize and PortSupportedFc4Types.
 *	PortSupportedSpeed, PortSpeed, and PortState are set to values
 *	indicating unknwon. NumberofDiscoveredPorts is 0.
 *	Most other values are determined using a GA_NXT request on the Name
 *	Server Directory Service.
 * @note For PortSupportedFc4Types and PortActive Fc4Types we do not follow
 *	FC-HBA Rev 10. We do not store them "little-endian" but "big-endian" as
 *	it is suggested by Editors Note 1.
 */
HBA_STATUS HBA_GetDiscoveredPortAttributes(HBA_HANDLE handle,
					   HBA_UINT32 portindex,
					   HBA_UINT32 discoveredportindex,
					   HBA_PORTATTRIBUTES *pPortattributes)
{
	HBA_STATUS status;
	int ret;
	struct vlib_adapter *adapter;
	struct vlib_port *port;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	status = revalidateRepository();
	if (HBA_STATUS_OK != status) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	if (portindex != 0) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_ILLEGAL_INDEX;
	}

	port = getPortByIndex(adapter, discoveredportindex);
	if (NULL == port) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_ILLEGAL_INDEX;
	}

	if (port->isInvalid) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_UNAVAILABLE;
	}

	status = sysfs_getDiscoveredPortAttributes(&pPortattributes, port);

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	return status;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_GetPortAttributesByWWN(HBA_HANDLE handle, HBA_WWN PortWWN,
				      HBA_PORTATTRIBUTES *pPortattributes)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup SupportedHBAAPIs
 * @brief Return statistics of an adapter port
 * @param handle to an opened adapter
 * @param portindex index of adapter port
 * @param pPortstatistics pointer to return statistics
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 * 	- HBA_STATUS_ERROR_ILLEGAL_INDEX if portindex is invalid
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note Parameter portindex _must_ be 0, since we have only one local port on
 *	our adapters.
 */
HBA_STATUS HBA_GetPortStatistics(HBA_HANDLE handle, HBA_UINT32 portindex,
				 HBA_PORTSTATISTICS *pPortstatistics)
{
	HBA_STATUS status;
	struct vlib_adapter *adapter;
	int ret;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	status = revalidateRepository();
	if (HBA_STATUS_OK != status) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	if (portindex != 0) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_ILLEGAL_INDEX;
	}

	status = sysfs_getPortStatistics(&pPortstatistics, adapter);

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	return status;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_GetFC4Statistics(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
				HBA_UINT8 FC4type,
				HBA_FC4STATISTICS *statistics)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}


/*
 *	FCP INFORMATION FUNCTIONS
 */


/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_GetBindingCapability(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
				    HBA_BIND_CAPABILITY *pFlags)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_GetBindingSupport(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
				 HBA_BIND_CAPABILITY *pFlags)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_SetBindingSupport(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
				 HBA_BIND_CAPABILITY flags)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup SupportedHBAAPIs
 * @brief Retrieve mappings between OS SCSI targets/units and FCP targets/units.
 * @param handle to an opened adapter
 * @param *pMapping pointer to return target mappings
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *	- HBA_STATUS_ERROR_MORE_DATA if there is not enough space in pMapping
 *	to return complete mapping information
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note An OSDeviceName is not provided for target mappings.
 * @note Additionally this function triggers creation of unit configuration for
 *	this adapter (see revalidateUnits()).
 */
HBA_STATUS HBA_GetFcpTargetMapping(HBA_HANDLE handle,
				   HBA_FCPTARGETMAPPING *pMapping)
{
	unsigned int i, j;
	struct vlib_adapter *adapter;
	struct vlib_port *port;
	struct vlib_unit *unit;
	HBA_UINT32 total, free;
	HBA_STATUS status;
	HBA_FCPSCSIENTRY *entry;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	status = revalidateRepository();
	if (HBA_STATUS_OK != status) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	if (revalidatePorts(adapter) < 0) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR;
	}

	port = getPortByIndex(adapter, 0);
	if (NULL == port) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR;
	}

	free = pMapping->NumberOfEntries;
	entry = pMapping->entry;
	total = 0;

	for (i = 0; i < adapter->ports.used; ++i, ++port) {
		if (port->isInvalid)
			continue;

		if (revalidateUnits(port) < 0) {
			VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
			return HBA_STATUS_ERROR;
		}

		unit = getUnitByIndex(port, 0);
		if (NULL == unit)
			continue;

		for (j = 0; j < port->units.used; ++j, ++unit) {
			if (unit->isInvalid)
				continue;

			++total;

			if (0 == free)
				/* although there is no space to return further
				   mappings, we still have to determine the
				   total number of mappings which must be
				   returned in the NumberOfEntries field */
				continue;

			strcpy(entry->ScsiId.OSDeviceName, "");
			entry->ScsiId.ScsiBusNumber = unit->channel;
			entry->ScsiId.ScsiTargetNumber = unit->target;
			entry->ScsiId.ScsiOSLun = unit->lun;
			snprintf(entry->ScsiId.OSDeviceName,
				sizeof(entry->ScsiId.OSDeviceName),
				"/dev/bsg/%d:%d:%d:%d",
				adapter->ident.host,
				unit->channel,
				unit->target,
				unit->lun);

			entry->FcpId.FcId = vlib_FCID_to_hbaFCID(port->did);
			vlib_wwn_to_HBA_WWN(port->wwnn,
					    &entry->FcpId.NodeWWN);
			vlib_wwn_to_HBA_WWN(port->wwpn,
					    &entry->FcpId.PortWWN);
			entry->FcpId.FcpLun = unit->fcLun;

			++entry;
			--free;
		}
	}

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	if (total > pMapping->NumberOfEntries)
		status = HBA_STATUS_ERROR_MORE_DATA;
	else
		status = HBA_STATUS_OK;

	pMapping->NumberOfEntries = total;

	return status;
}

/** @ingroup SupportedHBAAPIs
 * @brief Retrieve mappings between OS SCSI targets/units and FCP targets/units.
 * @param handle to an opened adapter
 * @param *pMapping pointer to return target mappings
 * @param *hbaPortWWN wwpn to identify the port on the adapter
 * @return see HBA_GetFcpTargetMapping
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note HBA_LUID is not provided for target mappings.
 * @note Our "adapters" have only one port, so the WWN parameter is
 *	superfluous. We only check if it matches to the adapter handle,
 * 	if yes, we call HBA_GetFcpTargetMapping
 */
HBA_STATUS HBA_GetFcpTargetMappingV2(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
				     HBA_FCPTARGETMAPPINGV2 *pMappingV2)
{
	struct vlib_adapter *adapter;
	wwn_t wwpn;
	int size;
	int i;
	HBA_STATUS status;
	HBA_FCPTARGETMAPPING *pMapping;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);
	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	vlib_HBA_WWN_to_wwn(&hbaPortWWN, &wwpn);
	if (wwpn != adapter->ident.wwpn) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_ILLEGAL_WWN;
	}
	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	size = pMappingV2->NumberOfEntries;
	pMapping = malloc(sizeof(HBA_FCPTARGETMAPPING) +
					sizeof(HBA_FCPSCSIENTRY) * size);
	if (pMapping == NULL)
		return HBA_STATUS_ERROR;

	pMapping->NumberOfEntries = size;

	status =  HBA_GetFcpTargetMapping(handle, pMapping);

	pMappingV2->NumberOfEntries = pMapping->NumberOfEntries;

	for (i = 0; i < size && i < pMapping->NumberOfEntries; i++) {
		memcpy(&pMappingV2->entry[i], &pMapping->entry[i],
				sizeof(HBA_FCPSCSIENTRY));
		pMappingV2->entry[i].LUID.buffer[0] = '\0';
	}

	free(pMapping);

	return status;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_GetFcpPersistentBinding(HBA_HANDLE handle,
				       HBA_FCPBINDING *pBinding)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_GetPersistentBindingV2(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
				      HBA_FCPBINDING2 *binding)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_SetPersistentBindingV2(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
				      HBA_FCPBINDING2 *binding)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_RemovePersistentBinding(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
				       HBA_FCPBINDING2 *binding)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_RemoveAllPersistentBindings(HBA_HANDLE handle,
					   HBA_WWN hbaPortWWN)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_GetFCPStatistics(HBA_HANDLE handle, const HBA_SCSIID *lunit,
				HBA_FC4STATISTICS *statistics)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/*
 *	SCSI INFORMATION FUNCTIONS
 */

/**
 * Is to be called by both, HBA_SendScsiInquiry and HBA_ScsiInquiryV2.
 * The difference is that HBA_SendScsiInquiry does not care for the return
 * value of RspBufferSize while HBA_ScsiInquiryV2 does. See the calls to
 * this function underneath.
 */
static HBA_STATUS _HBA_SendScsiInquiry(HBA_HANDLE handle, HBA_WWN PortWWN,
			       HBA_UINT64 fcLUN, HBA_UINT8 EVPD,
			       HBA_UINT32 PageCode,
			       void *pRspBuffer, HBA_UINT32 *RspBufferSize,
			       void *pSenseBuffer, HBA_UINT32 *SenseBufferSize)
{
	struct vlib_adapter *adapter;
	wwn_t wwpn;
	HBA_STATUS status;
	struct vlib_port *port;
	struct vlib_unit *unit;

	pSenseBuffer = NULL;
	*SenseBufferSize = 0;

	/* you need to be root to access /dev/sg* */
	if (getuid())
		return HBA_STATUS_ERROR;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);
	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	vlib_HBA_WWN_to_wwn(&PortWWN, &wwpn);
	port = getPortByWWPN(adapter, wwpn);
	if (port == NULL) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_ILLEGAL_WWN;
	}

	unit = getUnitByFcLun(port, fcLUN);
	if (unit == NULL) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_INVALID_LUN;
	}

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	status = sgutils_SendScsiInquiry(unit->sg_dev, EVPD, PageCode,
					pRspBuffer, RspBufferSize);

	return status;
}

/** @ingroup SupportedHBAAPIs
 * @brief Send a SCSI INQUIRY command to a FCP LUN.
 * @param handle to an opened adapter
 * @param PortWWN WWPN of the target port
 * @param fcLUN FCP LUN of the unit
 * @param EVPD Enhanced Vital Product Data
 * @param PageCode Vital Product Data page code if EVPD is set
 * @param *pRspBuffer pointer to return response data
 * @param RspBufferSize size of the response buffer
 * @param *pSenseBuffer pointer to return sense data on SCSI CHECK_CONDITION
 * @param SenseBufferSize size of the sense buffer
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *	- HBA_STATUS_ERROR_INVALID_LUN if there is no unit for the the specified
 *	fcLUN configured
 *	- HBA_STATUS_ERROR_MORE_DATA if there is not enough space in pRspBuffer
 *	- HBA_STATUS_SCSI_CHECK_CONDITION if a SCSI CHECK_CONDITION occurs
 *	- HBA_STATUS_ERROR_ARG if EVPD is neither 0 nor 1
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note
 *	- HBA_STATUS_ERROR_NOT_A_TARGET is not returned because zfcp just deals
 *	with SCSI target ports.
 *	- HBA_STATUS_ERROR_TARGET_BUSY is not returned because zfcp cannot
 *	detect SCSI command overlap situations in general.
 *	- ZFCP HBA API sends INQUIRY as untagged if the unit is not previously
 *	registered at the SCSI mid layer. If the device is already registered
 *	there, untagged/tagged is chosen as indicated in the associated
 *	Scsi_Device structure of the unit.
 * @note ZFCP HBA API sends INQUIRY as untagged if the unit is not previously
 *	registered at the mid layer. If the device is already registered there,
 *	untagged/tagged is chosen as indicated in the associated Scsi_Device
 *	structure of the unit.
 */
HBA_STATUS HBA_SendScsiInquiry(HBA_HANDLE handle, HBA_WWN PortWWN,
			       HBA_UINT64 fcLUN, HBA_UINT8 EVPD,
			       HBA_UINT32 PageCode,
			       void *pRspBuffer, HBA_UINT32 RspBufferSize,
			       void *pSenseBuffer, HBA_UINT32 SenseBufferSize)
{
	return _HBA_SendScsiInquiry(handle, PortWWN, fcLUN, EVPD, PageCode,
					pRspBuffer, &RspBufferSize,
					pSenseBuffer, &SenseBufferSize);
}

/** @ingroup SupportedHBAAPIs
 * @brief Send a SCSI INQUIRY command to a FCP LUN.
 * @param handle to an opened adapter
 * @param hbaPortWWN WWPN of the local adapter port
 * @param discoveredPortWWN WWPN of the target port
 * @param fcLUN FCP LUN of the unit
 * @param EVPD Enhanced Vital Product Data
 * @param PageCode Vital Product Data page code if EVPD is set
 * @param *pRspBuffer pointer to return response data
 * @param RspBufferSize size of the response buffer
 * @param *pSenseBuffer pointer to return sense data on SCSI CHECK_CONDITION
 * @param SenseBufferSize size of the sense buffer
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *	- HBA_STATUS_ERROR_INVALID_LUN if there is no unit for the the specified
 *	fcLUN configured
 *	- HBA_STATUS_ERROR_MORE_DATA if there is not enough space in pRspBuffer
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note
 *	- HBA_STATUS_ERROR_NOT_A_TARGET is not returned because zfcp just deals
 *	with SCSI target ports.
 *	- HBA_STATUS_ERROR_TARGET_BUSY is not returned because zfcp cannot
 *	detect SCSI command overlap situations in general.
 */
HBA_STATUS HBA_ScsiInquiryV2(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
			     HBA_WWN discoveredPortWWN, HBA_UINT64 fcLUN,
			     HBA_UINT8 CDB_Byte1, HBA_UINT8 CDB_Byte2,
			     void *pRspBuffer, HBA_UINT32 *pRspBufferSize,
			     HBA_UINT8 *pScsiStatus, void *pSenseBuffer,
			     HBA_UINT32 *pSenseBufferSize)
{
	*pScsiStatus = 0;
	return _HBA_SendScsiInquiry(handle, discoveredPortWWN, fcLUN,
					CDB_Byte1, CDB_Byte2,
					pRspBuffer, pRspBufferSize,
					pSenseBuffer, pSenseBufferSize);
}

/**
 * Is to be called by both, HBA_SendReportLUNs and HBA_ScsiReportLUNsV2.
 * The difference is that HBA_SendReportLUNs does not care for the return
 * value of RspBufferSize while HBA_ScsiReportLUNsV2 does. See the calls to
 * this function underneath.
 */
static HBA_STATUS _HBA_SendReportLUNs(HBA_HANDLE handle, HBA_WWN portWWN,
			      void *pRspBuffer, HBA_UINT32 *RspBufferSize,
			      void *pSenseBuffer, HBA_UINT32 *SenseBufferSize)
{
	struct vlib_adapter *adapter;
	wwn_t wwpn;
	HBA_STATUS status;
	struct vlib_port *port;
	char *sg_dev;
	int wlunattached = 0;

	pSenseBuffer = NULL;
	*SenseBufferSize = 0;

	/* you need to be root to access /dev/sg* */
	if (getuid())
		return HBA_STATUS_ERROR;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);
	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	vlib_HBA_WWN_to_wwn(&portWWN, &wwpn);
	port = getPortByWWPN(adapter, wwpn);
	if (port == NULL) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_ILLEGAL_WWN;
	}

	sg_dev = getSgDevFromPort(port);
	if (!sg_dev) {
		sg_dev = getAttachedWLUN(adapter, port);
		wlunattached = 1;
	}

	status = sgutils_SendReportLUNs(sg_dev, pRspBuffer, RspBufferSize);

	if (wlunattached)
		detachWLUN(adapter, port);

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	return status;
}

/** @ingroup SupportedHBAAPIs
 * @brief Send a SCSI REPORT LUNS command to a target.
 * @param handle to an opened adapter
 * @param portWWN WWPN of the target port
 * @param *pRspBuffer pointer to return response data
 * @param RspBufferSize size of the response buffer
 * @param *pSenseBuffer pointer to return sense data on SCSI CHECK_CONDITION
 * @param SenseBufferSize size of the sense buffer
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *	- HBA_STATUS_SCSI_CHECK_CONDITION if a SCSI CHECK_CONDITION occurs
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note
 * 	Lun Scanning only works if we have at least Lun 0 attached.
 * 	In all other cases we cannot scan the Luns (yet).
 * 	In addition, no SCSI sense data will be returned.
 */
HBA_STATUS HBA_SendReportLUNs(HBA_HANDLE handle, HBA_WWN portWWN,
			      void *pRspBuffer, HBA_UINT32 RspBufferSize,
			      void *pSenseBuffer, HBA_UINT32 SenseBufferSize)
{
	return _HBA_SendReportLUNs(handle, portWWN, pRspBuffer, &RspBufferSize,
						pSenseBuffer, &SenseBufferSize);
}
/** @ingroup SupportedHBAAPIs
 * @brief Send a SCSI REPORT LUNS command to a target.
 * @param handle to an opened adapter
 * @param portWWN WWPN of the local port
 * @param discoveredPortWWN WWPN of the target port
 * @param *pRspBuffer pointer to return response data
 * @param *RspBufferSize pointer to size of the response buffer
 * @param *pSenseBuffer pointer to return sense data on SCSI CHECK_CONDITION
 * @param *SenseBufferSize pointer to size of the sense buffer
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *	- HBA_STATUS_SCSI_CHECK_CONDITION if a SCSI CHECK_CONDITION occurs
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note
 * 	Lun Scanning only works if we have at least Lun 0 attached.
 * 	In all other cases we cannot scan the Luns (yet).
 * 	In addition, no SCSI sense data will be returned.
 * 	The only real difference to the V1 function is the additional
 * 	parameter for the local port. Since our "adapters" have only one port,
 * 	we can omit it.
 */
HBA_STATUS HBA_ScsiReportLUNsV2(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
				HBA_WWN discoveredPortWWN, void *pRspBuffer,
				HBA_UINT32 *pRspBufferSize,
				HBA_UINT8 *pScsiStatus, void *pSenseBuffer,
				HBA_UINT32 *pSenseBufferSize)
{
	return _HBA_SendReportLUNs(handle, discoveredPortWWN, pRspBuffer,
			pRspBufferSize, pSenseBuffer, pSenseBufferSize);
}

/**
 * Is to be called by both, HBA_SendReadCapacity and HBA_ScsiReadCapacityV2.
 * The difference is that HBA_SendReadCapacity does not care for the return
 * value of RspBufferSize while HBA_ScsiReadCapacityV2 does. See the calls to
 * this function underneath.
 */
static HBA_STATUS _HBA_SendReadCapacity(HBA_HANDLE handle, HBA_WWN portWWN,
				HBA_UINT64 fcLUN,
				void *pRspBuffer, HBA_UINT32 *RspBufferSize,
				void *pSenseBuffer, HBA_UINT32 *SenseBufferSize)
{
	int ret;
	struct vlib_adapter *adapter;
	struct zh_scsi_read_capacity;
	HBA_STATUS status;
	wwn_t wwpn;
	struct vlib_port *port;
	struct vlib_unit *unit;

	pSenseBuffer = NULL;
	*SenseBufferSize = 0;

	if (*RspBufferSize < READCAP10LEN)
		/* to small to hold the smallest response */
		return HBA_STATUS_ERROR_MORE_DATA;

	vlib_HBA_WWN_to_wwn(&portWWN, &wwpn);

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	status = revalidateRepository();
	if (HBA_STATUS_OK != status) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	port = getPortByWWPN(adapter, wwpn);
	if (port == NULL) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_ILLEGAL_WWN;
	}

	unit = getUnitByFcLun(port, fcLUN);
	if (unit == NULL) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return HBA_STATUS_ERROR_INVALID_LUN;
	}

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	status = sgutils_SendReadCap(unit->sg_dev, pRspBuffer, RspBufferSize);

	return status;
}
/** @ingroup SupportedHBAAPIs
 * @brief Send a SCSI READ CAPACITY command to a FCP LUN.
 * @param handle to an opened adapter
 * @param portWWN WWPN of the target port
 * @param fcLUN FCP LUN of the unit
 * @param *pRspBuffer pointer to return response data
 * @param RspBufferSize size of the response buffer
 * @param *pSenseBuffer pointer to return sense data on SCSI CHECK_CONDITION
 * @param SenseBufferSize size of the sense buffer
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *	- HBA_STATUS_ERROR_INVALID_LUN if there is no unit for the the specified
 *	fcLUN configured
 *	- HBA_STATUS_ERROR_MORE_DATA if there is not enough space in pRspBuffer
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note
 *	- HBA_STATUS_ERROR_NOT_A_TARGET is not returned because zfcp just deals
 *	with SCSI target ports.
 *	- HBA_STATUS_ERROR_TARGET_BUSY is not returned because zfcp cannot
 *	detect SCSI command overlap situations in general.
 *	- ZFCP HBA API sends READ CAPACITY via libsgutils. Sense date is not
 * 	supported that way, so the sensebuffer is always NULL
 */
HBA_STATUS HBA_SendReadCapacity(HBA_HANDLE handle, HBA_WWN portWWN,
				HBA_UINT64 fcLUN,
				void *pRspBuffer, HBA_UINT32 RspBufferSize,
				void *pSenseBuffer, HBA_UINT32 SenseBufferSize)
{
	return _HBA_SendReadCapacity(handle, portWWN, fcLUN, pRspBuffer,
				&RspBufferSize, pSenseBuffer, &SenseBufferSize);
}
/** @ingroup SupportedHBAAPIs
 * @brief Send a SCSI READ CAPACITY command to a FCP LUN.
 * @param handle to an opened adapter
 * @param portWWN WWPN of the target port
 * @param fcLUN FCP LUN of the unit
 * @param *pRspBuffer pointer to return response data
 * @param RspBufferSize size of the response buffer
 * @param *pSenseBuffer pointer to return sense data on SCSI CHECK_CONDITION
 * @param SenseBufferSize size of the sense buffer
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *	- HBA_STATUS_ERROR_INVALID_LUN if there is no unit for the the specified
 *	fcLUN configured
 *	- HBA_STATUS_ERROR_MORE_DATA if there is not enough space in pRspBuffer
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 * @note
 *	- HBA_STATUS_ERROR_NOT_A_TARGET is not returned because zfcp just deals
 *	with SCSI target ports.
 *	- HBA_STATUS_ERROR_TARGET_BUSY is not returned because zfcp cannot
 *	detect SCSI command overlap situations in general.
 *	- ZFCP HBA API sends READ CAPACITY via libsgutils. Sense date is not
 * 	supported that way, so the sensebuffer is always NULL
 */
HBA_STATUS HBA_ScsiReadCapacityV2(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
				  HBA_WWN discoveredPortWWN, HBA_UINT64 fcLUN,
				  void *pRspBuffer, HBA_UINT32 *pRspBufferSize,
				  HBA_UINT8 *pScsiStatus, void *pSenseBuffer,
				  HBA_UINT32 *pSenseBufferSize)
{
	*pScsiStatus = 0;
	return _HBA_SendReadCapacity(handle, discoveredPortWWN, fcLUN,
		pRspBuffer, pRspBufferSize, pSenseBuffer, pSenseBufferSize);
}


/*
 *	FABRIC MANAGEMENT FUNCTIONS
 */


/** @ingroup SupportedHBAAPIs
 * @brief Send a CT pass thru - a CT frame constructed in userspace directly
 *		to the HBA / SAN
 * @param handle to an opened adapter
 * @param *pReqBuffer pointer to CT frame
 * @param ReqBufferSize size of the request buffer
 * @param *pRspBuffer pointer to return response data
 * @param RspBufferSize size of the response buffer
 * @return 
 *      - HBA_STATUS_NOT_LOADED if library is not loaded
 *      - HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *      - HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *      - HBA_STATUS_ERROR_MORE_DATA if there is not enough space in pRspBuffer
 *      - HBA_STATUS_ERROR if any other internal error occurs
 *      - HBA_STATUS_OK on success.
 * @par Locks:
 *      lock/unlock of vlib_data.mutex
 */
HBA_STATUS HBA_SendCTPassThru(HBA_HANDLE handle, void *pReqBuffer,
			      HBA_UINT32 ReqBufferSize, void *pRspBuffer,
			      HBA_UINT32 RspBufferSize)
{
	HBA_STATUS status;
	struct vlib_adapter *adapter;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	status = revalidateRepository();
	if (HBA_STATUS_OK != status) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}
	status = sg_io_performCTPassThru(adapter, pReqBuffer, ReqBufferSize,
						pRspBuffer, RspBufferSize);
	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
	return status;
}

/** @ingroup SupportedHBAAPIs
 * @brief Send a CT pass thru - a CT frame constructed in userspace directly
 *              to the HBA / SAN
 * @param handle to an opened adapter
 * @param hbaPortWWN local port of adapter - not necessary in our case
 * @param *pReqBuffer pointer to CT frame
 * @param ReqBufferSize size of the request buffer
 * @param *pRspBuffer pointer to return response data
 * @param RspBufferSize size of the response buffer
 * @return
 *      - HBA_STATUS_NOT_LOADED if library is not loaded
 *      - HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *      - HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *      - HBA_STATUS_ERROR_MORE_DATA if there is not enough space in pRspBuffer
 *      - HBA_STATUS_ERROR if any other internal error occurs
 *      - HBA_STATUS_OK on success.
 * @par Locks:
 *      lock/unlock of vlib_data.mutex
 */
HBA_STATUS HBA_SendCTPassThruV2(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
				void *pReqBuffer, HBA_UINT32 ReqBufferSize,
				void *pRspBuffer, HBA_UINT32 *pRspBufferSize)
{
	return HBA_SendCTPassThru(handle, pReqBuffer, ReqBufferSize,
						pRspBuffer, *pRspBufferSize);
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_SetRNIDMgmtInfo(HBA_HANDLE handle, HBA_MGMTINFO *info)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_GetRNIDMgmtInfo(HBA_HANDLE handle, HBA_MGMTINFO *pInfo)
{
	HBA_STATUS status;
	struct vlib_adapter *adapter;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	status = revalidateRepository();
	if (HBA_STATUS_OK != status) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	memset(pInfo, 0, sizeof(HBA_MGMTINFO));

	/* all other fields not set */
	vlib_wwn_to_HBA_WWN(adapter->ident.wwnn, &pInfo->wwn);
	pInfo->unittype = 0x000000a; /* host identifier, see FC-LS-2 */
	pInfo->PortId = 1; /* only one port */
	pInfo->NumberOfAttachedNodes = 1; /* one for Nx ports, see FC-LS-2 */
	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	return HBA_STATUS_OK;
}

/** @ingroup SupportedHBAAPIs
 * @brief Send a RNID ELS to a port.
 * @param handle to an opened adapter
 * @param wwn of port to which to send RNID ELS
 * @param wwntype deprecated
 * @param *pRspBuffer pointer to return response data
 * @param *pRspBufferSize pointer to size of response buffer
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *	- HBA_STATUS_ERROR_MORE_DATA if there is not enough space in pRspBuffer
 *	and response data is truncated
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success (LS_ACC or LS_RJT).
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 */
HBA_STATUS HBA_SendRNID(HBA_HANDLE handle, HBA_WWN wwn, HBA_WWNTYPE wwntype,
			void *pRspBuffer, HBA_UINT32 *pRspBufferSize)
{
	HBA_STATUS status;
	int ret;
	struct vlib_adapter *adapter;
	struct vlib_port *port;
	wwn_t portwwn;

	if (!pRspBuffer || *pRspBufferSize < 0)
		return HBA_STATUS_ERROR_ARG;

	/* you need to be root to access /dev/* */
	if (getuid())
		return HBA_STATUS_ERROR;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	status = revalidateRepository();
	if (HBA_STATUS_OK != status) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	vlib_HBA_WWN_to_wwn(&wwn, &portwwn);

	status = sg_io_sendRNID(adapter, portwwn, pRspBuffer, *pRspBufferSize);

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	if (status == HBA_STATUS_ERROR_ELS_REJECT)
		status = HBA_STATUS_OK;

	return status;
}

/** @ingroup SupportedHBAAPIs
 * @brief Send a RNID ELS to a port.
 * @param handle to an opened adapter
 * @param hbaPortWWN local port of adapter - not necessary in our case
 * @param wwn of port to which to send RNID ELS
 * @param wwntype deprecated
 * @param *pRspBuffer pointer to return response data
 * @param *pRspBufferSize pointer to size of response buffer
 * @return
 *      - HBA_STATUS_NOT_LOADED if library is not loaded
 *      - HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *      - HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *      - HBA_STATUS_ERROR_MORE_DATA if there is not enough space in pRspBuffer
 *      and response data is truncated
 *      - HBA_STATUS_ERROR if any other internal error occurs
 *      - HBA_STATUS_OK on success (LS_ACC or LS_RJT).
 * @par Locks:
 *      lock/unlock of vlib_data.mutex
 * @note this function just calls the V1 version above so the new functionality
 *       offered by V2 is not supported
 */
HBA_STATUS HBA_SendRNIDV2(HBA_HANDLE handle, HBA_WWN hbaPortWWN,
			  HBA_WWN destWWN, HBA_UINT32 destFCID,
			  HBA_UINT32 NodeIdDataFormat, void *pRspBuffer,
			  HBA_UINT32 *pRspBufferSize)
{
	return HBA_SendRNID(handle, destWWN, 0, pRspBuffer, pRspBufferSize);
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_SendRPL(HBA_HANDLE handle, HBA_WWN hbaPortWWN, HBA_WWN agent_wwn,
		       HBA_UINT32 agent_domain, HBA_UINT32 portIndex,
		       void *pRspBuffer, HBA_UINT32 *pRspBufferSize)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_SendRPS(HBA_HANDLE handle, HBA_WWN hbaPortWWN, HBA_WWN agent_wwn,
		       HBA_UINT32 agent_domain, HBA_WWN object_wwn,
		       HBA_UINT32 object_port_number, void *pRspBuffer,
		       HBA_UINT32 *pRspBufferSize)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_SendSRL(HBA_HANDLE handle, HBA_WWN hbaPortWWN, HBA_WWN wwn,
		       HBA_UINT32 domain, void *pRspBuffer,
		       HBA_UINT32 *pRspBufferSize)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_SendLIRR(HBA_HANDLE handle, HBA_WWN hbaPortWWN, HBA_WWN destWWN,
			HBA_UINT8 function, HBA_UINT8 type, void *pRspBuffer,
			HBA_UINT32 *pRspBufferSize)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_SendRLS(HBA_HANDLE handle, HBA_WWN hbaPortWWN, HBA_WWN destWWN,
		       void *pRspBuffer, HBA_UINT32 *pRspBufferSize)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup SupportedHBAAPIs
 * @brief Return events for an adapter from the event queue.
 * @param handle to an opened adapter
 * @param *pEventBuffer pointer to return events
 * @param *pEventCount pointer to size of event buffer (in event records)
 * @return
 *	- HBA_STATUS_NOT_LOADED if library is not loaded
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 */
HBA_STATUS HBA_GetEventBuffer(HBA_HANDLE handle, HBA_EVENTINFO *pEventBuffer,
			      HBA_UINT32 *pEventCount)
{
	struct vlib_adapter *adapter;
	HBA_STATUS status;
	int i = 0;
	struct vlib_event *event;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	status = revalidateRepository();
	if (HBA_STATUS_OK != status) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}

	adapter = getAdapterByHandle(handle, &status);
	if (NULL == adapter) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return status;
	}


	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);

	for (i = 0; i < *pEventCount; i++) {
		event = popEvent(adapter);
		if (!event)
			break;
		memcpy(pEventBuffer + i, &event->hbaapi_event,
							sizeof(HBA_EVENTINFO));
	}
	*pEventCount = i;

	return HBA_STATUS_OK;
}
