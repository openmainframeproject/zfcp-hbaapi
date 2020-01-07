/*
 * Copyright IBM Corp. 2003,2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:		Sven Schuetz <sven@de.ibm.com>
 *
 * File:		vlib_sysfs.c
 *
 * Description:
 * All calls that use the sysfs.
 *
 */

 /**
 * @file vlib_sysfs.c
 * @brief All calls that use the sysfs.
 */

#include "vlib.h"

/* internal helper functions */

/**
 * @brief add a  port to the adapters repos
 * @param *adapter the adapter to which the add the port to
 * @param *name the unique name of the remote port as in fc_remote_ports
 * @return
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 * 	vlib_data.mutex must be held
 */
static HBA_STATUS addPortByName(struct vlib_adapter *adapter, char *name)
{
	char path[PATH_MAX];
	char attr[ATTR_MAX];
	int ret;
	struct vlib_port port;

	snprintf(path, PATH_MAX, "/sys/class/fc_remote_ports/%s", name);

	strcpy(port.name, name);
	sscanf(name, "rport-%d:%d-%d", &port.host, &port.channel, &port.target);

	ret = sfhelper_getProperty(path, "node_name", attr);
	if (!ret)
		port.wwnn = strtoull(attr, NULL, 16);
	ret = sfhelper_getProperty(path, "port_name", attr);
	if (!ret)
		port.wwpn = strtoull(attr, NULL, 16);
	ret = sfhelper_getProperty(path, "port_id", attr);
	if (!ret)
		port.did = strtoul(attr, NULL, 16);

	addPortToRepos(adapter, &port);
	return HBA_STATUS_OK;
}

/**
 * @brief add an adapter to the adapters repos
 * @param *dev_path the sysfs device as seen under
 * 		/sys/devices/css0/x.x.xxx/x.x.xxxx
 * @return
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 * 	vlib_data.mutex must be held
 */
static HBA_STATUS addAdapterByDevPath(char *dev_path)
{
	char *fc_host_name;
	struct vlib_adapter a;
	char attr[ATTR_MAX];
	char classpath[PATH_MAX];
	int ret;
	sfhelper_dir *dir;

	dir = sfhelper_opendir(dev_path);
	if (dir == NULL)
		return HBA_STATUS_OK;

	ret = sfhelper_getProperty(dev_path, "online", attr);
	if (!ret && strncmp(attr, "0", 1) == 0)
		return HBA_STATUS_ERROR_INVALID_HANDLE;

	while (fc_host_name = sfhelper_getNextDirEnt(dir)) {
		if (strncmp(fc_host_name, "host", 4) == 0)
			break;
	}

	if (fc_host_name == NULL)
		/* adapter is offline, no fc_host entry */
		return HBA_STATUS_ERROR_UNAVAILABLE;

	a.ident.host = strtoul(fc_host_name + 4, NULL, 0);
	sfhelper_closedir(dir);

	strcpy(a.ident.sysfsPath, dev_path);

	snprintf(classpath, PATH_MAX, "%s/%s", FC_HOST_PATH, fc_host_name);
	dir = sfhelper_opendir(classpath);
	if (dir == NULL)
		/* adapter is offline, no fc_host entry */
		return HBA_STATUS_ERROR_UNAVAILABLE;
	sfhelper_closedir(dir);

	/* devno is at the end of the path, e.g.
	 * /sys/devices/css0/0.0.0010/0.0.5923, so we copy the last 9 bytes
	 * (including the NULL termination) */
	memcpy(a.ident.bus_dev_name, dev_path + strlen(dev_path) - DEVNO_LENGTH,
							DEVNO_LENGTH + 1);
	strcpy(a.ident.class_dev_name, fc_host_name);

	memcpy(&a.ident.devid, a.ident.bus_dev_name, 8);

	ret = sfhelper_getProperty(classpath, "node_name", attr);
	if (!ret)
		a.ident.wwnn = strtoull(attr, NULL, 16);
	ret = sfhelper_getProperty(classpath, "port_name", attr);
	if (!ret)
		a.ident.wwpn = strtoull(attr, NULL, 16);
	ret = sfhelper_getProperty(classpath, "port_id", attr);
	if (!ret)
		a.ident.did = strtoul(attr, NULL, 16);

	addAdapterToRepos(&a);

	return HBA_STATUS_OK;
}

/**
 * @brief Retrieve adapter attributes.
 * @param **pPortattributes, HBA_ADAPTERATTRIBUTES to be filled
 * @param *adapter to work with
 * @return
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 *
 * This function reads attributes from sysfs to fill in the required
 * information.
 */
static HBA_STATUS getPortAttributes(HBA_PORTATTRIBUTES **pPortattributes,
					char *classpath)
{
	char attr[ATTR_MAX];
	int ret;

	/* Worldwide Port and Node Name */
	ret = sfhelper_getProperty(classpath, "node_name", attr);
	if (!ret)
		vlib_wwn_to_HBA_WWN(strtoull(attr, NULL, 16),
					&(*pPortattributes)->NodeWWN);
	ret = sfhelper_getProperty(classpath, "port_name", attr);
	if (!ret)
		vlib_wwn_to_HBA_WWN(strtoull(attr, NULL, 16),
					&(*pPortattributes)->PortWWN);

	/* PortFcId */
	ret = sfhelper_getProperty(classpath, "port_id", attr);
	if (!ret)
		(*pPortattributes)->PortFcId = strtoul(attr, NULL, 16);

	/* Port Type */
	ret = sfhelper_getProperty(classpath, "port_type", attr);
	if (!ret)
		(*pPortattributes)->PortType = vlibCharToIntPortType(attr);

	/* Port State */
	ret = sfhelper_getProperty(classpath, "port_state", attr);
	if (attr)
		(*pPortattributes)->PortState =  vlibCharToIntPortState(attr);

	/* Supported Classes */
	ret = sfhelper_getProperty(classpath, "supported_classes", attr);
	if (!ret)
		(*pPortattributes)->PortSupportedClassofService =
						vlibCharToIntCOS(attr);
	/* Supported FC4 types, we only support SCSI FCP which is
	 * represented by 0x0000 0100 in Word 1 */
	(*pPortattributes)->PortSupportedFc4Types.bits[2] = 0x1;

	/* 0 when port down, otherwise same as above */
	if ((*pPortattributes)->PortState == HBA_PORTSTATE_ONLINE)
		(*pPortattributes)->PortActiveFc4Types.bits[2] = 0x1;

	/* Symbolic Name is empty */

	/* OSDeviceName is empty, we do not have a device */

	/* Supported port speeds */
	ret = sfhelper_getProperty(classpath, "supported_speeds", attr);
	if (!ret)
		(*pPortattributes)->PortSupportedSpeed =
						vlibCharToIntPortSpeed(attr);

	/* port speed */
	ret = sfhelper_getProperty(classpath, "speed", attr);
	if (!ret)
		(*pPortattributes)->PortSpeed = vlibCharToIntPortSpeed(attr);

	/* max frame size */
	ret = sfhelper_getProperty(classpath, "maxframe_size", attr);
	if (!ret)
		(*pPortattributes)->PortMaxFrameSize = atoi(attr);

	/* FabricName is empty */

	return HBA_STATUS_OK;
}

/* functions directly called from vlib.c or vlib_aux.c */


 /**
 * @brief Read and store all discovered ports of an adapter
 * @param *adapter pointer to the adapter in which we are interested
 * @return
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 * 	vlib_data.mutex must be held
 */
HBA_STATUS sysfs_createAndReadConfigPorts(struct vlib_adapter *adapter)
{
	HBA_STATUS status;
	sfhelper_dir *dir;
	char path[PATH_MAX];
	char *portname;

	if (adapter->ident.devid == 0)
		return HBA_STATUS_ERROR;

	snprintf(path, PATH_MAX, "%s/host%d", adapter->ident.sysfsPath,
							adapter->ident.host);
	dir = sfhelper_opendir(path);
	if (dir == NULL)
		return HBA_STATUS_ERROR;

	while (portname = sfhelper_getNextDirEnt(dir)) {
		if (strncmp(portname, "rport", 5) == 0)
			addPortByName(adapter, portname);
	}

	sfhelper_closedir(dir);
	return HBA_STATUS_OK;
}

/**
 * @brief Read all adapters from /sys/bus/ccw/drivers/zfcp and add them
 * 	to the repository
 * @return
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 * @par Locks:
 * 	vlib_data.mutex must be held
 */
HBA_STATUS sysfs_createAndReadConfigAdapter()
{
	HBA_STATUS status;
	int ret, a;
	sfhelper_dir *dir;
	char *dev_path;
	char path[PATH_MAX];

	dir = sfhelper_opendir(ZFCP_SYSFS_PATH);
	if (dir == NULL)
		return HBA_STATUS_OK;

	/* loop dir entries to find devices of form x.x.xxxx */
	while (dev_path = sfhelper_getNextDirEnt(dir)) {
		if (sscanf(dev_path, "%x.%x.%x", &a, &a, &a) != 3)
			continue; /* no match, try next one */
		sprintf(path, "%s/%s", ZFCP_SYSFS_PATH, dev_path);

		/* memory will be allocated, needs to be freed */
		dev_path = realpath(path, NULL);
		addAdapterByDevPath(dev_path);
		free(dev_path);
	}

	sfhelper_closedir(dir);

	ret = revalidateAdapters();
	if (ret < 0)
		status = HBA_STATUS_ERROR;
	else {
		vlib_data.isValid = 1;
		status = HBA_STATUS_OK;
	}

	return status;
}

/**
 * @brief Get unit configuration information for a port.
 * @param *port for which unit configuration is received
 * @return
 *	- -1 on error
 *	- 0 on success.
 * @par Locks:
 *	vlib_data.mutex must be held
 *
 * This function creates and reads private configuration events for all
 * configured units of an port. It enlarges the array where unit data is
 * stored and processes the generated configuration events.
 */
int sysfs_getUnitsFromPort(struct vlib_port *port)
{
	char path[PATH_MAX];
	char unitPath[PATH_MAX], unitPath2[PATH_MAX];
	char attr[ATTR_MAX];
	char *sg, *sg2;
	struct vlib_unit unit;
	struct vlib_adapter *adapter;
	sfhelper_dir *dir, *sg_dir, *sg_dir2;
	char *dirent;
	int ret;
	uint32_t sgindex;

	adapter = getAdapterByHostNo(port->host);
	if (!adapter)
		return HBA_STATUS_ERROR;

	snprintf(path, PATH_MAX, "%s/host%d/%s", adapter->ident.sysfsPath,
					adapter->ident.host, port->name);

	dir = sfhelper_opendir(path);
	if (dir == NULL)
		return HBA_STATUS_ERROR;

	/* loop dir entries to find targets */
	while (dirent = sfhelper_getNextDirEnt(dir)) {
		if (strncmp(dirent, "target", 6) == 0)
			break;
	}
	if (dirent == NULL)
		/* no units, no error*/
		return 0;

	sfhelper_closedir(dir);

	strcat(path, "/");
	strcat(path, dirent);

	dir = sfhelper_opendir(path);
	if (dir == NULL)
		/* no units, no error */
		return 0;

	while (dirent = sfhelper_getNextDirEnt(dir)) {
		unit.sg_dev[0] = '\0';
		ret = sscanf(dirent, "%d:%d:%d:%d", &unit.host,
					&unit.channel, &unit.target, &unit.lun);
		if (ret != 4)
			continue;
		strcpy(unitPath, path);
		strcat(unitPath, "/");
		strcat(unitPath, dirent);
		ret = sfhelper_getProperty(unitPath, "fcp_lun", attr);
		if (!ret)
			unit.fcLun = strtoull(attr, NULL, 16);
		sg_dir = sfhelper_opendir(unitPath);
		if (sg_dir == NULL)
			continue;
		while (sg = sfhelper_getNextDirEnt(sg_dir)) {
			ret = sscanf(sg, "scsi_generic:%s", &unit.sg_dev);
			if (ret == 1)
				/* successful match */
				break;
			/* search match without CONFIG_SYSFS_DEPRECATED[_V2] */
			if (strncmp(sg, "scsi_generic", 13 /* full */) != 0)
				continue;
			snprintf(unitPath2, PATH_MAX, "%s/%s", unitPath, sg);
			sg_dir2 = sfhelper_opendir(unitPath2);
			if (sg_dir2 == NULL)
				continue;
			while (sg2 = sfhelper_getNextDirEnt(sg_dir2)) {
				if (sscanf(sg2, "sg%u", &sgindex) != 1)
					continue;
				snprintf(unit.sg_dev, sizeof(unit.sg_dev),
					 "%s", sg2);
				/* successful match */
				break;
			}
			sfhelper_closedir(sg_dir2);
		}
		sfhelper_closedir(sg_dir);
		addUnitToRepos(port, &unit);
	}
	sfhelper_closedir(dir);
	return 0;
}

/**
 * @brief Retrieve adapter attributes.
 * @param **pPortattributes, HBA_ADAPTERATTRIBUTES to be filled
 * @param *adapter to work with
 * @return
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 *
 * This function reads attributes from sysfs to fill in the required
 * information.
 */
HBA_STATUS sysfs_getDiscoveredPortAttributes(HBA_PORTATTRIBUTES **pAttrs,
							struct vlib_port *port)
{
	sfhelper_dir *dir;
	char path[PATH_MAX];

	memset(*pAttrs, 0, sizeof(HBA_PORTATTRIBUTES));

	snprintf(path, PATH_MAX, "/sys/class/fc_remote_ports/%s", port->name);
	dir = sfhelper_opendir(path);

	if (!dir)
		return HBA_STATUS_ERROR_UNAVAILABLE;
	sfhelper_closedir(dir);

	getPortAttributes(pAttrs, path);

	/* not applicable to remote ports at the moment */
	memset(&(*pAttrs)->PortActiveFc4Types, 0, sizeof(HBA_FC4TYPES));
	memset(&(*pAttrs)->PortSupportedFc4Types, 0, sizeof(HBA_FC4TYPES));

	return HBA_STATUS_OK;
}

/**
 * @brief Retrieve adapter attributes.
 * @param **pPortattributes, HBA_ADAPTERATTRIBUTES to be filled
 * @param *adapter to work with
 * @return
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 *
 * This function reads attributes from sysfs to fill in the required
 * information.
 */
HBA_STATUS sysfs_getAdapterPortAttributes(HBA_PORTATTRIBUTES **pAttrs,
						struct vlib_adapter *adapter)
{
	sfhelper_dir *dir;
	char path[PATH_MAX];
	char *dirent;

	if (adapter->ident.devid == 0)
		return HBA_STATUS_ERROR_UNAVAILABLE;

	memset(*pAttrs, 0, sizeof(HBA_PORTATTRIBUTES));

	snprintf(path, PATH_MAX, "/sys/class/fc_host/host%d",
							adapter->ident.host);
	dir = sfhelper_opendir(path);
	if (!dir)
		return HBA_STATUS_ERROR_UNAVAILABLE;
	sfhelper_closedir(dir);

	getPortAttributes(pAttrs, path);
	snprintf((*pAttrs)->OSDeviceName, sizeof((*pAttrs)->OSDeviceName),
				"/dev/bsg/fc_host%d", adapter->ident.host);

	snprintf(path, PATH_MAX, "%s/host%d", adapter->ident.sysfsPath,
						adapter->ident.host);

	dir = sfhelper_opendir(path);
	if (!dir)
		return HBA_STATUS_ERROR_UNAVAILABLE;

	while (dirent = sfhelper_getNextDirEnt(dir)) {
		if (strncmp(dirent, "rport", 5) == 0) {
			(*pAttrs)->NumberofDiscoveredPorts++;
			addPortByName(adapter, dirent);
		}
	}
	sfhelper_closedir(dir);

	return HBA_STATUS_OK;
}

/**
 * @brief Retrieve adapter attributes.
 * @param **pAdapterattributes, HBA_ADAPTERATTRIBUTES to be filled
 * @param *adapter to work with
 * @return
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 *
 * This function reads attributes from sysfs to fill in the required
 * information.
 */
HBA_STATUS sysfs_getAdapterAttributes(HBA_ADAPTERATTRIBUTES **pAttrs,
						struct vlib_adapter *adapter)
{
	char classpath[PATH_MAX], attr[ATTR_MAX];
	int ret, a;

	memset(*pAttrs, 0, sizeof(HBA_ADAPTERATTRIBUTES));

	snprintf(classpath, PATH_MAX, "/sys/class/fc_host/host%d",
							adapter->ident.host);
	/* Manufacturer */
	strcpy((*pAttrs)->Manufacturer, "IBM");

	/* Serial Number */
	ret = sfhelper_getProperty(classpath, "serial_number", attr);
	if (!ret)
		strcpy((*pAttrs)->SerialNumber, attr);

	/* Model */
	ret = sfhelper_getProperty(adapter->ident.sysfsPath, "card_version",
									attr);
	if (!ret) {
		a = strtoul(attr, NULL, 16);
		strcpy((*pAttrs)->ModelDescription,
				"zSeries/System z Fibre Channel Adapter ");
		switch (a) {
		case 1:
			strcpy((*pAttrs)->Model, "Ficon Adapater");
			strcpy((*pAttrs)->ModelDescription, "Hydra 1.5");
			break;
		case 2:
			strcpy((*pAttrs)->Model, "Ficon Express Adapater");
			strcpy((*pAttrs)->ModelDescription, "Hydra 1.75");
			break;
		case 3:
			strcpy((*pAttrs)->Model, "Ficon Express2 Adapater");
			strcpy((*pAttrs)->ModelDescription,
						"Ficon-3 with 2 Gbit/s");
			break;
		case 4:
			strcpy((*pAttrs)->Model, "Ficon Express2.5 Adapater");
			strcpy((*pAttrs)->ModelDescription,
						"Ficon-3 with 4 Gbit/s");
			break;
		default:
			strcpy((*pAttrs)->Model, "Unknown");
			strcpy((*pAttrs)->ModelDescription, "Unknown");
		}
	}

	/* World Wide Node Name */
	vlib_wwn_to_HBA_WWN(adapter->ident.wwnn, &(*pAttrs)->NodeWWN);

	/* NodeSymbolicName not set */

	/* DriverVersion not set */

	/* Hardware Version */
	ret = sfhelper_getProperty(adapter->ident.sysfsPath, "hardware_version",
									attr);
	if (!ret)
		strcpy((*pAttrs)->HardwareVersion, attr);

	/* OptionROMVersion not set */

	/* VendorSpecificID will be the devno a.b.cccc
	 * a will be stored in the first byte
	 * b will be stored in the second byte
	 * cccc will be stored in the last two bytes */
	sscanf(adapter->ident.bus_dev_name, "%hhd.%hhd.%hx",
			(char *)&(*pAttrs)->VendorSpecificID,
			(char *)(&(*pAttrs)->VendorSpecificID) + 1,
			(char *)(&(*pAttrs)->VendorSpecificID) + 2);

	/* Firmware (here LIC) Version */
	ret = sfhelper_getProperty(adapter->ident.sysfsPath, "lic_version",
									attr);
	if (!ret)
		strcpy((*pAttrs)->FirmwareVersion, attr);

	/* Number of adapters ... always one */
	(*pAttrs)->NumberOfPorts = 1; /* always 1 */

	/* Driver name */
	strcpy((*pAttrs)->DriverName, "zfcp");

	return HBA_STATUS_OK;
}

/**
 * @brief Retrieve adapter port statistics
 * @param **pPortstatistics, HBA_PORTSTATISTICS to be filled
 * @param *adapter to work with
 * @return
 *	- HBA_STATUS_ERROR if any other internal error occurs
 * 	- HBA_STATUS_OK on success.
 *
 * This function reads attributes from sysfs to fill in the required
 * information.
 */
HBA_STATUS sysfs_getPortStatistics(HBA_PORTSTATISTICS **pS,
						struct vlib_adapter *adapter)
{
	char attr[ATTR_MAX];
	char path[PATH_MAX];
	sfhelper_dir *dir;
	int ret;

	memset(*pS, 0, sizeof(HBA_PORTSTATISTICS));

	snprintf(path, PATH_MAX, "/sys/class/fc_host/host%d/statistics",
							adapter->ident.host);
	dir = sfhelper_opendir(path);

	if (!dir)
		return HBA_STATUS_ERROR_UNAVAILABLE;
	sfhelper_closedir(dir);

	ret = sfhelper_getProperty(path, "seconds_since_last_reset", attr);
	if (!ret)
		(*pS)->SecondsSinceLastReset = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "tx_frames", attr);
	if (!ret)
		(*pS)->TxFrames = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "tx_words", attr);
	if (!ret)
		(*pS)->TxWords = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "rx_frames", attr);
	if (!ret)
		(*pS)->RxFrames = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "rx_words", attr);
	if (!ret)
		(*pS)->RxWords = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "lip_count", attr);
	if (!ret)
		(*pS)->LIPCount = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "nos_count", attr);
	if (!ret)
		(*pS)->NOSCount = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "error_frames", attr);
	if (!ret)
		(*pS)->ErrorFrames = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "dumped_frames", attr);
	if (!ret)
		(*pS)->DumpedFrames = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "link_failure_count", attr);
	if (!ret)
		(*pS)->LinkFailureCount = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "loss_of_sync_count", attr);
	if (!ret)
		(*pS)->LossOfSyncCount = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "loss_of_signal_count", attr);
	if (!ret)
		(*pS)->LossOfSignalCount = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "prim_seq_protocol_err_count", attr);
	if (!ret)
		(*pS)->PrimitiveSeqProtocolErrCount = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "invalid_tx_word_count", attr);
	if (!ret)
		(*pS)->InvalidTxWordCount = strtoull(attr, NULL, 16);

	ret = sfhelper_getProperty(path, "invalid_crc_count", attr);
	if (!ret)
		(*pS)->InvalidCRCCount = strtoull(attr, NULL, 16);

	return HBA_STATUS_OK;
}
