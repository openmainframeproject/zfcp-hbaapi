/*
 * Copyright IBM Corp. 2003,2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:		Andreas Herrmann
 *			Stefan Voelkel
 *
 * File:		vlib_aux.h
 *
 * Description:
 * Auxiliary functions used in the library.
 *
 */

#ifndef _VLIB_AUX_H_
#define _VLIB_AUX_H_
/**
 * @file vlib_aux.h
 * @brief Auxiliary functions used in the library.
 */

#include "vlib.h"

#define VLIB_GROW_UNITS 8
#define VLIB_GROW_PORTS 4
#define VLIB_GROW_ADAPTERS 2

#ifdef min
# undef min
#endif
#define min(a, b) (((a) < (b)) ? (a) : (b))

/*
 * function declarations
 */

struct vlib_adapter *getAdapterByIndex(uint32_t);
struct vlib_adapter *getAdapterByHandle(HBA_HANDLE, HBA_STATUS *);
struct vlib_adapter *getAdapterByDevid(devid_t);
struct vlib_adapter *getAdapterByHostNo(unsigned short);
struct vlib_port *getPortByIndex(const struct vlib_adapter *, const uint32_t);
struct vlib_port *getPortByWWPN(const struct vlib_adapter *, const wwn_t);
struct vlib_unit *getUnitByIndex(const struct vlib_port *, const uint32_t);
struct vlib_unit *getUnitByFcLun(const struct vlib_port *, uint64_t);

int addAdapterToRepos(struct vlib_adapter *);
int addPortToRepos(struct vlib_adapter *, struct vlib_port *);
int addUnitToRepos(struct vlib_port *, struct vlib_unit *);

HBA_STATUS getAdapterConfig(void);
int getUnitsFromPort(struct vlib_port *);

int findIndexByName(char *);
HBA_HANDLE openAdapterByIndex(HBA_UINT32);
char *getSgDevFromPort(struct vlib_port *);
char *getAttachedWLUN(struct vlib_adapter *, struct vlib_port *);
void detachWLUN(struct vlib_adapter *, struct vlib_port *);

int revalidateAdapters(void);
int updateAdapter(struct vlib_adapter *adapter);
void doCloseAdapter(struct vlib_adapter *);
void closeAllAdapters(void);

HBA_PORTTYPE vlibCharToIntPortType(char *);
HBA_PORTSTATE vlibCharToIntPortState(char *);
HBA_PORTSPEED vlibCharToIntPortSpeed(char *);
HBA_COS vlibCharToIntCOS(char *);

/*
 * inline functions
 */

/**
 * @brief Convert uint64_t to HBA_WWN -- hide ill-favoured type cast.
 * @param wwn WWN stored as uint64_t (to be converted)
 * @param *hba pointer to return WWN of type HBA_WWN
 */
static inline void vlib_wwn_to_HBA_WWN(uint64_t wwn, HBA_WWN *hba)
{
	*((uint64_t *)(&hba->wwn)) = wwn;
}

/**
 * @brief Convert HBA_WWN to uint64_t -- hide ill-favoured type cast.
 * @param *hba pointer WWN of type HBA_WWN (to be converted)
 * @param *wwn pointer to return WWN as uint64_t
 */
static inline void vlib_HBA_WWN_to_wwn(HBA_WWN *hba, uint64_t *wwn)
{
	*wwn = *((uint64_t *)hba->wwn);
}

/**
 * @brief Convert a FC DID to a FC-HBA PortFcId.
 * @param fcid a FC DID (uses 3 least significant bytes)
 * @return a PortFcId as specified in FC-HBA (uses 3 most significant bytes)
 */
static inline uint32_t vlib_FCID_to_hbaFCID(uint32_t fcid)
{
	return fcid << 8;
}

/**
 * @brief Convert a FC-HBA PortFcId to a FC DID.
 * @param fcid PortFcId as specified in FC-HBA (uses 3 most significant bytes)
 * @return a FC DID (uses 3 least significant bytes)
 */
static inline uint32_t vlib_hbaFCID_to_FCID(uint32_t fcid)
{
	return fcid >> 8;
}


/**
 * @brief Mark all adapters in repository as invalid.
 * @par Locks:
 * 	vlib_data.mutex must be held
 */
static inline void invalidateAllAdapters(void)
{
	unsigned int i;
	struct vlib_adapter *adapter;

	adapter = getAdapterByIndex(0);
	for (i = 0; i < vlib_data.adapters.used; ++i, ++adapter)
		adapter->isInvalid = 1;
}

/**
 * @brief Mark repositroy of library as invalid. This is appropriate if a
 *	loss of	events is detected.
 * @par Locks:
 *	lock/unlock of vlib_data.mutex
 */
static inline void markRepositoryInvalid(void)
{
	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	vlib_data.isValid = 0;

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
}

#endif /* _VLIB_AUX_H_ */
