/*
 * Copyright IBM Corp. 2003,2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:	Andreas Herrmann
 *		Stefan Voelkel
 *
 * File:	vlib_callbacks.c
 *
 * Description:
 * This file contains the implementations of callback registration
 * HBA API routines.
 *
 */

/**
 * @file vlib_callbacks.c
 * @brief Implementations of %callback registration HBA API functions.
 * @note We only include vlib.h which in turn should include all other
 * necessary header files.
 *
 * This file contains the implementations of callback registration
 * HBA API routines.
 */

#include "vlib.h"

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS HBA_RemoveCallback(HBA_CALLBACKHANDLE callbackHandle)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS
HBA_RegisterForAdapterAddEvents(void (*pCallback) (void *, HBA_WWN, HBA_UINT32),
				void *pUserData,
				HBA_CALLBACKHANDLE *pCallbackHandle)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS
HBA_RegisterForAdapterEvents(void (*pCallback) (void *, HBA_WWN, HBA_UINT32),
			     void *pUserData, HBA_HANDLE handle,
			     HBA_CALLBACKHANDLE *pCallbackHandle)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS
HBA_RegisterForAdapterPortEvents(void (*pCallback)
				 (void *, HBA_WWN, HBA_UINT32, HBA_UINT32),
				 void *pUserData, HBA_HANDLE handle,
				 HBA_WWN PortWWN,
				 HBA_CALLBACKHANDLE *pCallbackHandle)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS
HBA_RegisterForAdapterPortStatEvents(void (*pCallback)
				     (void *, HBA_WWN, HBA_UINT32),
				     void *pUserData, HBA_HANDLE handle,
				     HBA_WWN PortWWN, HBA_PORTSTATISTICS stats,
				     HBA_UINT32 statType,
				     HBA_CALLBACKHANDLE *pCallbackHandle)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS
HBA_RegisterForTargetEvents(void (*pCallback)
			    (void *, HBA_WWN, HBA_WWN, HBA_UINT32),
			    void *pUserData, HBA_HANDLE handle,
			    HBA_WWN hbaPortWWN, HBA_WWN discoveredPortWWN,
			    HBA_CALLBACKHANDLE *pCallbackHandle,
			    HBA_UINT32 allTargets)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}

/** @ingroup UnSupportedHBAAPIs
 * @return HBA_STATUS_ERROR_NOT_SUPPORTED
 * @note This function is currently not supported.
 */
HBA_STATUS
HBA_RegisterForLinkEvents(void (*pCallback)
			  (void *, HBA_WWN, HBA_UINT32, void *, HBA_UINT32),
			  void *pUserData, void *pRLIRBuffer,
			  HBA_UINT32 RLIRBufferSize, HBA_HANDLE handle,
			  HBA_CALLBACKHANDLE *pCallbackHandle)
{
	return HBA_STATUS_ERROR_NOT_SUPPORTED;
}
