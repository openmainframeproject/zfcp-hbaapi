/*
 * Copyright IBM Corp. 2003, 2018
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:		Andreas Herrmann
 *			Stefan Voelkel
 *			Sven Schuetz <sven@de.ibm.com>
 *
 * File:		vlib_aux.c
 *
 * Description:
 * Auxiliary functions used in the library.
 *
 */

/**
 * @file vlib_aux.c
 * @brief Auxiliary functions used in the library.
 */
#include "vlib.h"


/* static function declarations */
static int block_assertSize
	(struct block *, const size_t, const size_t, const size_t);
static void *block_addItem(struct block *, size_t, size_t);


/**
 * @brief Assert that there is space for at least num elements in the block.
 * @param *block pointer to struct block
 *	(contains array of elements of passed size)
 * @param size of the contained structure
 * @param num minimum number of elements in the block
 * @param grow chunk size (The array grows by chunks of size (grow * size).)
 * @return
 *	- -ENOMEM if out of memory
 *	- total number of elements in the block
 */
static int block_assertSize(struct block *block, const size_t size,
				   const size_t num, const size_t grow)
{
	char *c;
	size_t needed;

	if (num <= block->allocated)
		return block->allocated;

	needed = num - block->allocated;
	if (needed % grow)
		needed += grow - (needed % grow);

	c = realloc(block->data, (block->allocated + needed) * size);
	if (NULL == c) {
		VLIB_PERROR(ENOMEM, "ERROR");
		return -ENOMEM;
	}

	memset(c + block->allocated * size, 0, needed * size);

	block->data = c;
	block->allocated += needed;

	return block->allocated;
}

/**
 * @brief Add a new item to a block.
 * @param *block pointer to struct block
 *	(contains array of elements of passed size)
 * @param size of the contained structure
 * @param grow chunk size (The array grows by chunks of size (grow * size).)
 * @return
 *	- NULL on error
 *	- pointer to new item on success
 *
 * If the new item does not fit in the array, the array is enlarged.
 */
static void *block_addItem(struct block *block, size_t size, size_t grow)
{
	int ret;
	void *item;

	ret = block_assertSize(block, size, block->used + 1, grow);
	if (ret < 0)
		return NULL;

	item = ((char *) block->data) + (size * block->used);

	++block->used;

	return item;
};

/**
 * @brief Free the array contained in a struct block.
 * @param *block pointer to a struct block
 */
void block_free(struct block *block)
{
	free(block->data);
	block->data = NULL;
	block->used = block->allocated = 0;
}

/**
 * @brief Get an adapter by its index.
 * @param index of the adapter
 * @return
 *	- NULL if index is out of range
 *	- pointer to found adapter on success
 * @parm Locks:
 *	vlib_data.mutex must be held
 */
struct vlib_adapter *getAdapterByIndex(uint32_t index)
{
	if (index >= vlib_data.adapters.used)
		return NULL;
	else
		return &((struct vlib_adapter *)vlib_data.adapters.data)[index];
}

/**
 * @brief Get an adapter by its handle.
 * @param handle of the adapter
 * @param *status pointer to return error status code
 * @return
 *	- NULL on error (*status contains error status code)
 *	- pointer to adapter structure on success
 * @par Locks:
 *	vlib_data.mutex must be held
 *
 * If NULL is returned *status contains an error status code which should be
 * checked by the calling function. If non-NULL is returned *status is
 * HBA_STATUS_OK.
 * Possible error status codes are:
 *	- HBA_STATUS_ERROR_INVALID_HANDLE if handle is invalid
 *	- HBA_STATUS_ERROR_UNAVAILABLE if adapter is unavailable
 */
struct vlib_adapter *getAdapterByHandle(HBA_HANDLE handle, HBA_STATUS *status)
{
	struct vlib_adapter *adapter;

	if (VLIB_INVALID_HANDLE == handle) {
		*status = HBA_STATUS_ERROR_INVALID_HANDLE;
		return NULL;
	}

	adapter = getAdapterByIndex(handle - 1);
	if (NULL == adapter) {
		*status = HBA_STATUS_ERROR_INVALID_HANDLE;
	} else {
		if (adapter->isInvalid) {
			*status = HBA_STATUS_ERROR_UNAVAILABLE;
			adapter = NULL;
		} else {
			*status = HBA_STATUS_OK;
		}
	}

	return adapter;
}

/**
 * @brief Get an adapter by its devid.
 * @param devid of the adapter
 * @return
 * 	- NULL if no such adapter exists
 * 	- pointer to found adapter
 * @par Locks:
 * 	vlib_data.mutex must be held
 */
struct vlib_adapter *getAdapterByDevid(devid_t devid)
{
	struct vlib_adapter *adapter;
	unsigned int i;

	adapter = getAdapterByIndex(0);
	if (NULL == adapter)
		return NULL;

	for (i = 0; i < vlib_data.adapters.used; ++i, ++adapter) {
		if (adapter->ident.devid == devid)
			return adapter;
	}

	return NULL;
}

/**
 * @brief Get an adapter by SCSI Host number as in sysfs
 * @param SCSI host number of the adapter
 * @return
 * 	- NULL if no such adapter exists
 * 	- pointer to found adapter
 * @par Locks:
 * 	vlib_data.mutex must be held
 */
struct vlib_adapter *getAdapterByHostNo(unsigned short host)
{
	struct vlib_adapter *adapter;
	unsigned int i;

	adapter = getAdapterByIndex(0);
	if (NULL == adapter)
		return NULL;

	for (i = 0; i < vlib_data.adapters.used; ++i, ++adapter) {
		if (adapter->ident.host == host)
			return adapter;
	}

	return NULL;
}

/**
 * @brief Get a port by its index.
 * @param *adapter to which the port belongs
 * @param index of the port
 * @return
 * 	- NULL if index is out of range
 * 	- pointer to found port on success
 * @par Locks:
 * 	vlib_Data.mutex must be held
 */
struct vlib_port *
getPortByIndex(const struct vlib_adapter *adapter, const uint32_t index)
{
	if (index >= adapter->ports.used)
		return NULL;

	return &((struct vlib_port *)adapter->ports.data)[index];
}

/**
 * @brief Get a port by its WWPN.
 * @param *adapter to which the port belongs
 * @param wwpn of the port
 * @return
 * 	- NULL if no port with such a WWPN exists
 * 	- pointer to found port on success
 * @par Locks:
 * 	vlib_data.mutex must be held
 */
struct vlib_port*
getPortByWWPN(const struct vlib_adapter *adapter, const wwn_t wwpn)
{
	unsigned int i;
	struct vlib_port *port;

	port = getPortByIndex(adapter, 0);
	if (NULL == port)
		return NULL;

	for (i = 0; i < adapter->ports.used; ++i, ++port) {
		if (wwpn == port->wwpn)
			return port;
	}

	return NULL;
}

/**
 * @brief Get an unit by its index.
 * @param *port to which the unit belongs
 * @param index of the unit
 * @return
 * 	- NULL if index is out of range
 * 	- pointer to found unit on success
 * @par Locks:
 * 	vlib_data.mutex must be held
 */
struct vlib_unit *getUnitByIndex(const struct vlib_port *port,
					       const uint32_t index)
{
	if (index >= port->units.used)
		return NULL;

	return &((struct vlib_unit *)port->units.data)[index];
}

/**
 * @brief Get an unit by its fclun
 * @param *port to which the unit belongs
 * @param index of the unit
 * @return
 * 	- NULL if index is out of range
 * 	- pointer to found unit on success
 * @par Locks:
 * 	vlib_data.mutex must be held
 */
struct vlib_unit *getUnitByFcLun(const struct vlib_port *port, uint64_t fcLun)
{
	unsigned int i;
	struct vlib_unit *unit;

	unit = getUnitByIndex(port, 0);
	if (NULL == unit)
		return NULL;

	for (i = 0; i < port->units.used; ++i, ++unit) {
		if (fcLun == unit->fcLun)
			return unit;
	}

	return NULL;
}


/**
 * @brief Check if an unit specified in an unit event is already stored in the
 *	repository.
 * @param *port to which this unit belongs
 * @param *event unit add event
 * @return
 * 	- NULL if no such unit exists for the port in the repository
 *	- pointer to found unit
 */
static struct vlib_unit *getUnitFromRepos(struct vlib_port *port,
					       struct vlib_unit *unit)
{
	unsigned int i;
	struct vlib_unit *unitLoc;

	unitLoc = getUnitByIndex(port, 0);
	for (i = 0; i < port->units.used; ++i, ++unitLoc) {
		if ((unitLoc->fcLun == unit->fcLun))
			return unitLoc;
	}

	return NULL;
}

/**
 * @brief Add a unit to the repository.
 * @param *port to which the unit data should be added, if NULL is passed
 *	the port should be identified by the data in the event
 * @param *unit unit to be aded
 * @return
 *	- -1 on error
 *	- 0 on success
 * @par Locks:
 *	vlib_data.mutex must be held
 *
 * If the unit specified in the event is already stored in the repository
 * it is marked as valid.
 */
int addUnitToRepos(struct vlib_port *port,
			    struct vlib_unit *unit)
{
	struct vlib_unit *unitLoc;

	unitLoc = getUnitFromRepos(port, unit);
	if (NULL != unitLoc) {
		unitLoc->isInvalid = 0;
		return 0;
	}

	unitLoc = block_addItem(&port->units, sizeof(*unit), VLIB_GROW_UNITS);
	if (NULL == unitLoc)
		return -1;

	memcpy(unitLoc, unit, sizeof(struct vlib_unit));

	return 0;
}

/**
 * @brief Check if a port specified  is already stored in the repository.
 * @param *adapter to which this port belongs
 * @param *port the new port for whose existence is checked
 * @return
 * 	- NULL if no such port exists for the adapter in the repository
 *	- pointer to found port
 * @par Locks:
 *	vlib_data.mutex must be held
 */
static struct vlib_port *getPortFromRepos(struct vlib_adapter *adapter,
							char *sysfs_name)
{
	unsigned int i;
	struct vlib_port *portLoc;

	portLoc = getPortByIndex(adapter, 0);
	for (i = 0; i < adapter->ports.used; ++i, ++portLoc) {
		if (strcmp(portLoc->name, sysfs_name) == 0)
			return portLoc;
	}
	return NULL;
}

/**
 * @brief Add a port from to the repository.
 * @param *adapter to which the port data should be added, if NULL is passed
 *	the adapter should be identified by the data in the event
 * @param *port port to be added
 * @return
 *	- -1 on error
 *	- 0 on success
 * @par Locks:
 *	vlib_data.mutex must be held
 *
 * If the port specified in the event is already stored in the repository
 * it is marked as valid.
 */
int addPortToRepos(struct vlib_adapter *adapter,
			    struct vlib_port *port)
{
	struct vlib_port *portLoc;

	portLoc = getPortFromRepos(adapter, port->name);
	if (NULL != portLoc) {
		port->isInvalid = 0;
		return 0;
	}

	portLoc = block_addItem(&adapter->ports, sizeof(*port),
							VLIB_GROW_PORTS);
	if (NULL == portLoc)
		return -1;

	portLoc->wwpn = port->wwpn;
	portLoc->wwnn = port->wwnn;
	portLoc->did = port->did;
	portLoc->host = port->host;
	portLoc->channel = port->channel;
	portLoc->target = port->target;
	strcpy(portLoc->name, port->name);

	return 0;
}

/**
 * @brief Check if an adapter specified in an event is already stored in the
 *	repository.
 * @param *bus_dev_name name of adapter as in the sysfs bus dev name
 * @return
 *	- pointer to found adapter
 *	- NULL if adapter is not yet in the repository
 * @par Locks:
 *	vlib_data.mutex must be held
 */
static struct vlib_adapter *getAdapterFromRepos(char *bus_dev_name)
{
	unsigned int i;
	struct vlib_adapter *adapterLoc;

	adapterLoc = getAdapterByIndex(0);
	for (i = 0; i < vlib_data.adapters.used; ++i, ++adapterLoc) {
		if (strcmp(adapterLoc->ident.bus_dev_name, bus_dev_name) == 0)
			return adapterLoc;
	}

	return NULL;
}

/**
 * @brief Add an adapter to the repository.
 * @param *adapter adapter
 * @return
 *	- -1 on error
 *	- 0 on success
 * @par Locks:
 *	vlib_data.mutex must be held
 *
 * If the adapter specified in the event is already stored in the repository
 * it is marked as valid.
 */
int addAdapterToRepos(struct vlib_adapter *adapter)
{
	struct vlib_adapter *adapterLoc;

	adapterLoc = getAdapterFromRepos(adapter->ident.bus_dev_name);
	if (NULL != adapterLoc) {
		adapterLoc->isInvalid = 0;
		return 0;
	}

	adapterLoc = block_addItem(&vlib_data.adapters, sizeof(*adapter),
				VLIB_GROW_ADAPTERS);
	if (NULL == adapterLoc)
		return -1;

	adapterLoc->ident.devid = adapter->ident.devid;
	adapterLoc->ident.wwpn = adapter->ident.wwpn;
	adapterLoc->ident.wwnn = adapter->ident.wwnn;
	adapterLoc->ident.host = adapter->ident.host;
	strcpy(adapterLoc->ident.bus_dev_name, adapter->ident.bus_dev_name);
	strcpy(adapterLoc->ident.class_dev_name,
					adapter->ident.class_dev_name);
	strcpy(adapterLoc->ident.sysfsPath, adapter->ident.sysfsPath);
	adapterLoc->isInvalid = 0;
	adapterLoc->handle = VLIB_INVALID_HANDLE;

	return 0;
}

/**
 * @brief Update information about ports and units of an adapter.
 * @param *adapter to be updated
 * @return
 *	- -1 on error
 *	- 0 on success
 * @par Locks:
 * 	vlib_data.mutex must be held
 * @note Additionally this function triggers creation of unit configuration for
 *	this adapter (see getUnitsFromPort()).
 */
int updateAdapter(struct vlib_adapter *adapter)
{
	unsigned int i;
	int ret;
	struct vlib_port *port;

	if (adapter->isInvalid)
		return 0;

	ret = sysfs_createAndReadConfigPorts(adapter);
	if (0 == ret) {
		port = getPortByIndex(adapter, 0);
		for (i = 0; i < adapter->ports.used; ++i, ++port) {
			ret = sysfs_getUnitsFromPort(port);
			if (ret < 0)
				break;
		}
	}

	return ret;
}

/**
 * @brief Revalidate adapters in the repository.
 * @return
 *	- -1 on error
 *	- 0 on success
 * @par Locks:
 * 	vlib_data.mutex must be held
 *
 * Port and unit configuration data is only updated if it was already generated
 * before. Generation of port and unit configuration information is triggered in
 * HBA_GetAdapterPortAttributes() and HBA_GetFcpTargetMapping(), resp.
 */
int revalidateAdapters(void)
{
	unsigned int i;
	int ret;
	struct vlib_adapter *adapter;

	adapter = getAdapterByIndex(0);
	for (i = 0; i < vlib_data.adapters.used; ++i, ++adapter) {
		if (adapter->isInvalid) {
			doCloseAdapter(adapter);
		} else {
			if (adapter->ports.allocated) {
				ret = updateAdapter(adapter);
				if (ret)
					return ret;
			}
		}
	}

	return 0;
}

/**
 * @brief Find an adapter index by name.
 * @param name the name of the adapter
 * @return
 *	- -1 on error.
 *	- index of adapter on success
 * @parm Locks:
 *	vlib_data.mutex must be held
 * @note If adapter.isInvalid we do not return an adapter index - although
 *	the name might be generated. At the moment this is not an issue,
 *	because currently this flag is not really set for adapters within
 *	configured within zfcp. But it might be an issue in the future, if
 *	adapters can be removed from within zfcp.
 */
int findIndexByName(char *name)
{
	char c[VLIB_ADAPTERNAME_LEN];
	HBA_UINT32 i;
	struct vlib_adapter *adapter;

	adapter = getAdapterByIndex(0);
	for (i = 0; i < vlib_data.adapters.used; ++i, ++adapter) {
		if (!adapter->isInvalid) {
			snprintf(c, VLIB_ADAPTERNAME_LEN, "%s%u",
				 VLIB_ADAPTERNAME_PREFIX, i);

			if (strncmp(name, c, VLIB_ADAPTERNAME_LEN) == 0)
				return i;
		}
	}

	return -1;
}

/**
 * @brief Open an adapter by index.
 * @param index of the adapter
 * @return
 *	- VLIB_INVALID_HANDLE if index is invalid
 *	- handle to the adapter on success
 * @par Locks:
 *	vlib_data.mutex must be held
 *
 * If compiled as a vendor library, we shall only use the lower 16 Bit of the
 * handle.
 */
HBA_HANDLE openAdapterByIndex(HBA_UINT32 index)
{
	struct vlib_adapter *adapter;

#ifdef HBA_VENDOR_LIBRARY
	if (index >= 0xFFFF)
		return VLIB_INVALID_HANDLE;
#endif

	adapter = getAdapterByIndex(index);
	if (NULL == adapter)
		return VLIB_INVALID_HANDLE;

	if (adapter->handle == VLIB_INVALID_HANDLE)
		adapter->handle = index + 1;

	init_event_queue(adapter);

	return adapter->handle;
}

/**
 * @brief Close an adapter in the repository.
 * @param *adapter pointer to the adapter to be closed
 * @par Locks:
 *	vlib_data.mutex must be held
 *
 * This function frees all allocated memory for the ports and units
 * of this adapter and invalidates the adapter handle.
 */
void doCloseAdapter(struct vlib_adapter *adapter)
{
	unsigned int i;
	struct vlib_port *port;

	adapter->handle = VLIB_INVALID_HANDLE;

	port = getPortByIndex(adapter, 0);
	if (NULL != port) {
		for (i = 0; i < adapter->ports.used; ++i, ++port)
			block_free(&port->units);

		block_free(&adapter->ports);
	}

	free_event_queue(adapter);
}

/**
 * @brief Close all adapters in the repository.
 * @par Locks:
 *	vlib_data.mutex must be held
 *
 * This function frees all allocated memory for the adapters.
 */
void closeAllAdapters(void)
{
	unsigned int i;
	struct vlib_adapter *a;

	a = getAdapterByIndex(0);
	for (i = 0; i < vlib_data.adapters.used; ++i, ++a)
		doCloseAdapter(a);

	block_free(&vlib_data.adapters);
}

/**
 * @brief Map the result of a port type string from sysfs to an int
 * @param char* containing the port type
 *
 * @note This function maps the port types from the sysfs attribute (which are
 * 	strings) to their number as defined in hbaapi.h
 */
HBA_PORTTYPE vlibCharToIntPortType(char *portType)
{
	if (strncmp(portType, "NPort", 5) == 0)
		return HBA_PORTTYPE_NPORT;
	else if (strncmp(portType, "NLPort", 6) == 0)
		return HBA_PORTTYPE_NLPORT;
	else if (strncmp(portType, "LPort", 5) == 0)
		return HBA_PORTTYPE_LPORT;
	else if (strncmp(portType, "Point-To-Point", 14) == 0)
		return HBA_PORTTYPE_PTP;
	else if (strncmp(portType, "Other", 5) == 0)
		return HBA_PORTTYPE_OTHER;
	else if (strncmp(portType, "Not Present", 11) == 0)
		return HBA_PORTTYPE_NOTPRESENT;
	else
		return HBA_PORTTYPE_UNKNOWN;
}

/**
 * @brief Map the result of a port state string from sysfs to an int
 * @param char* containing the port state
 *
 * @note This function maps the port states from the sysfs attribute (which are
 * 	strings) to their number as defined in hbaapi.h
 */
HBA_PORTSTATE vlibCharToIntPortState(char *portState)
{
	if (strncmp(portState, "Online", 6) == 0)
		return HBA_PORTSTATE_ONLINE;
	else if (strncmp(portState, "Offline", 7) == 0)
		return HBA_PORTSTATE_OFFLINE;
	else if (strncmp(portState, "Linkdown", 8) == 0)
		return HBA_PORTSTATE_LINKDOWN;
	else if (strncmp(portState, "Bypassed", 8) == 0)
		return HBA_PORTSTATE_BYPASSED;
	else if (strncmp(portState, "Diagnostics", 11) == 0)
		return HBA_PORTSTATE_DIAGNOSTICS;
	else if (strncmp(portState, "Error", 5) == 0)
		return HBA_PORTSTATE_ERROR;
	else if (strncmp(portState, "Loopback", 8) == 0)
		return HBA_PORTSTATE_LOOPBACK;
	else
		return HBA_PORTSTATE_UNKNOWN;
}

/**
 * @defgroup HBAPortSpeeds HBA Port Speeds
 *
 * Conditionally define HBA port speeds not defined in FC-HBA Revision 14.
 */
/*@{*/
#ifndef HBA_PORTSPEED_32GBIT
#define HBA_PORTSPEED_32GBIT 0x00000040
#endif
#ifndef HBA_PORTSPEED_128GBIT
#define HBA_PORTSPEED_128GBIT 0x00000200
#endif
#ifndef HBA_PORTSPEED_64GBIT
#define HBA_PORTSPEED_64GBIT 0x00000400
#endif
#ifndef HBA_PORTSPEED_256GBIT
#define HBA_PORTSPEED_256GBIT 0x00000800
#endif
/*@}*/

/**
 * @brief Maps the result of a port speed int to the HBA_PORTSPEED int
 * @param int holding the port speed
 *
 * @note This function maps the port speed in integer form (e.g. 1) to the
 * 	HBA API flag (e.g. HBA_PORTSPEED_1GBIT)
 */
HBA_PORTSPEED vlibIntToSymbolPortSpeed(int speed)
{
	switch (speed) {
	case 0:
		return 0;
		break;
	case 1:
		return HBA_PORTSPEED_1GBIT;
		break;
	case 2:
		return HBA_PORTSPEED_2GBIT;
		break;
	case 4:
		return HBA_PORTSPEED_4GBIT;
		break;
	case 8:
		return HBA_PORTSPEED_8GBIT;
		break;
	case 10:
		return HBA_PORTSPEED_10GBIT;
		break;
	case 16:
		return HBA_PORTSPEED_16GBIT;
		break;
	case 32:
		return HBA_PORTSPEED_32GBIT;
		break;
	case 64:
		return HBA_PORTSPEED_64GBIT;
		break;
	case 128:
		return HBA_PORTSPEED_128GBIT;
		break;
	case 256:
		return HBA_PORTSPEED_256GBIT;
		break;
	default:
		return HBA_PORTSPEED_UNKNOWN;
		break;
	}
}


/**
 * @brief Map the result of a port speed string to the HBA_PORTSPEED int
 * @param char* containing the port speed string
 *
 * @note This function maps the port speed from the sysfs (which is in the
 * 	form "1 Gbit, 2 Gbit" to an int storing a flag for each port speed
 */
HBA_PORTSPEED vlibCharToIntPortSpeed(char *pS)
{
	/* currently 6 speeds possible */
	int s1, s2, s3, s4, s5, s6;
	int r;
	int speed;

	speed = 0;
	s1 = s2 = s3 = s4 = s5 = s6 = 0;

	r = sscanf(pS, "%d Gbit, %d Gbit, %d Gbit, %d Gbit, %d Gbit, %d Gbit",
						&s1, &s2, &s3, &s4, &s5, &s6);

	speed |= vlibIntToSymbolPortSpeed(s1);
	speed |= vlibIntToSymbolPortSpeed(s2);
	speed |= vlibIntToSymbolPortSpeed(s3);
	speed |= vlibIntToSymbolPortSpeed(s4);
	speed |= vlibIntToSymbolPortSpeed(s5);
	speed |= vlibIntToSymbolPortSpeed(s6);

	return speed;
}

/**
 * @brief Maps the number of a class of service to its bit flag according
 * 	to FC-GS-4
 * @param int holding the class number
 *
 * @note Maps the number of a class of service to its bit flag according
 * 	to FC-GS-4
 */
int vlibCOStoFlag(int class)
{
	switch (class) {
	/* bit 0 is for class F, which is not supported */
	case 1:
		return 2; /* bit 1 is set */
		break;
	case 2:
		return 4; /* bit 2 is set */
		break;
	case 3:
		return 8; /* bit 3 is set */
		break;
	case 4:
		return 16; /* bit 4 is set */
		break;
	case 6:
		return 64; /* bit 6 is set */
		break;
	default:
		return 0;
		break;
	}
}

/**
 * @brief Map the result of a class of service string to an int
 * @param char* containing the class of service string
 *
 * @note This function maps the class of service from sysfs which is in the
 * 	form "Class2, Class3" to its integer representation via bit flags
 */
HBA_COS vlibCharToIntCOS(char *s)
{
	/* currently 5 classes possible */
	int s1, s2, s3, s4, s6;
	int r;
	HBA_COS cos;

	cos = 0;
	s1 = s2 = s3 = s4 = s6 = 0;

	r = sscanf(s, "Class %d, Class %d, Class %d, Class %d, Class %d",
						&s1, &s2, &s3, &s4, &s6);

	cos |= vlibCOStoFlag(s1);
	cos |= vlibCOStoFlag(s2);
	cos |= vlibCOStoFlag(s3);
	cos |= vlibCOStoFlag(s4);
	cos |= vlibCOStoFlag(s6);

	return cos;
}

/**
 * @brief Get the first sg device from an adapter
 * @param port* Pointer to a port
 * @note This function looks for the first lun belonging to the first port
 * 	of the adapter and returns its sg device
 */
char *getSgDevFromPort(struct vlib_port *port)
{
	struct vlib_unit *unit;


	if (revalidateUnits(port) < 0)
		return NULL;

	unit = getUnitByIndex(port, 0);
	if (unit == NULL)
		return NULL;

	return unit->sg_dev;
}

#define INTERVAL	10000000
#define RETRIES		100

/**
 * @brief Try to attach the report luns wlun and return its name as in "/dev"
 * @param adapter* Pointer to an adapter
 * @param port* Pointer to a port
 * @note This function issues a system call to attach lun0
 */
char *getAttachedWLUN(struct vlib_adapter *adapter, struct vlib_port *port)
{
	char s[128];
	int status;
	struct timespec t;
	char *sg_dev = NULL;
	int count = 0;

	sprintf(s, "echo 0x%lx > /sys/bus/ccw/drivers/zfcp/%s/0x%lx/unit_add",
		REPORTLUNS_WLUN, adapter->ident.bus_dev_name, port->wwpn);
	system(s);

	t.tv_nsec = INTERVAL;
	t.tv_sec = 0;

	sg_dev = getSgDevFromPort(port);
	while (!sg_dev && count < RETRIES) {
		nanosleep(&t, NULL);
		sg_dev = getSgDevFromPort(port);
		count++;
	}
	return sg_dev;
}

/**
 * @brief Try to detach lun 0
 * @param adapter* Pointer to an adapter
 * @param port* Pointer to a port
 * @note This function issues a system call to detach lun0
 */
void detachWLUN(struct vlib_adapter *adapter, struct vlib_port *port)
{
	char s[128];

	struct vlib_unit *unit;

	revalidateUnits(port);
	unit = getUnitByIndex(port, 0);

	if (unit) {
		sprintf(s, "echo 1 > /sys/bus/scsi/devices/%d:%d:%d:%d/delete",
				unit->host, unit->channel, unit->target,
				REPORTLUNS_WLUN_DEC);
		system(s);
	}

	sprintf(s,
		"echo 0x%lx > /sys/bus/ccw/drivers/zfcp/%s/0x%lx/unit_remove",
		REPORTLUNS_WLUN, adapter->ident.bus_dev_name, port->wwpn);
	system(s);
	revalidateUnits(port);
}
