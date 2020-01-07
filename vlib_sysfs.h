/*
 * Copyright IBM Corp. 2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:		Sven Schuetz <sven@de.ibm.com>
 * 			contains code from vlib_aux.h
 * 			by Andreas Herrmann and Stefan Voelkel
 *
 * File:		vlib_sysfs.h
 *
 * Description:
 * Function declarations and inline functions that use the sysfs.
 *
 */

#ifndef _VLIB_SYSFS_H_
#define _VLIB_SYSFS_H_


#define ZFCP_SYSFS_PATH "/sys/bus/ccw/drivers/zfcp"
#define FC_HOST_PATH "/sys/class/fc_host"

#define ATTR_MAX 80 /* all attributes are only one line */
#define DEVNO_LENGTH 8  /* x.x.xxxx -> 8 chars */

 /**
 * @file vlib_sysfs.h
 * @brief All calls that need the sysfs
 */

HBA_STATUS sysfs_createAndReadConfigPorts(struct vlib_adapter *);
HBA_STATUS sysfs_createAndReadConfigAdapter();
HBA_STATUS sysfs_getDiscoveredPortAttributes(HBA_PORTATTRIBUTES **,
						struct vlib_port *);
HBA_STATUS sysfs_getAdapterPortAttributes(HBA_PORTATTRIBUTES **,
						struct vlib_adapter *);
HBA_STATUS sysfs_getPortStatistics(HBA_PORTSTATISTICS **,
						struct vlib_adapter *);
int sysfs_getUnitsFromPort(struct vlib_port *);
void sysfs_waitForSgDev(char *);

/**
 * @brief Check status of the repository, and possibly revalidate it.
 * @return
 * 	- HBA_STATUS_ERROR_NOT_LOADED if not loaded
 * 	- HBA_STATUS_ERROR on revalidation error
 * 	- HBA_STATUS_OK on success
 * @par Locks:
 * 	vlib_data.mutex must be held
 *
 * Function getAdapterConfig() is called if the repository is not valid, thus
 * revalidating it.
 */
static inline HBA_STATUS revalidateRepository(void)
{
	if (!vlib_data.isLoaded)
		return HBA_STATUS_ERROR;

	if (!vlib_data.isValid)	{
		if (sysfs_createAndReadConfigAdapter() != HBA_STATUS_OK)
			return HBA_STATUS_ERROR;
	}

	return HBA_STATUS_OK;
}

/**
 * @brief Revalidate ports of an adapter in the repository.
 * @param *adapter for which ports should be revalidated
 * @return
 * 	- -1 on error
 * 	- 0 on success
 * @par Locks:
 *	vlib_data.mutex must be held
 *
 * This function might trigger the creation of port configuration information
 * for an adapter.
 */
static inline int revalidatePorts(struct vlib_adapter *adapter)
{
	if (0 == adapter->ports.allocated)
		return sysfs_createAndReadConfigPorts(adapter);

	return 0;
}

/**
 * @brief Revalidate units of an adapter and port in the repository.
 * @param *adapter to which the port belongs
 * @param *port for which the units should be revalidated
 * @return
 * 	- -1 on error
 * 	- 0 on success
 * @par Locks:
 *	vlib_data.mutex must be held
 *
 * This function might trigger the creation of unit configuration information
 * for an adapter and port.
 */
static inline int revalidateUnits(struct vlib_port *port)
{
	if (0 == port->units.allocated)
		return sysfs_getUnitsFromPort(port);

	return 0;
}

#endif /*_VLIB_SYSFS_H_*/
