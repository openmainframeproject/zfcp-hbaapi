/*
 * zfcp_util.h
 *
 * Common definitions for Fibre Channel utilities.
 * 
 * Copyright IBM Corp. 2009, 2018.
 *
 *  Author(s): Swen Schillig <swen@vnet.ibm.com>
 */

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#ifndef VLIB_ADAPTERNAME_LEN
#define VLIB_ADAPTERNAME_LEN 256
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE getpagesize()
#endif

#define CT_IU_CODE_OFFSET	8
#define CT_IU_PREAMBLE_SIZE	sizeof(struct ct_iu_preamble)

#define FC_TAG_NPORT	0x0001
#define FC_TAG_WWPN	0x0002

/* fabric configuration server request command codes */
#define FCS_RCC_GIEL	0x0101
#define FCS_RCC_GDID	0x0112
#define FCS_RCC_GIELN	0x0115
#define FCS_RCC_GIEIL	0x0117
#define FCS_RCC_GPL	0x0118
#define FCS_RCC_GPPN	0x0122
#define FCS_RCC_GAPNL	0x0124
#define FCS_RCC_GPS	0x0126
#define FCS_RCC_FPNG	0x0401

/* name server request */
#define NS_RCC_GFF_ID	0x011F
#define NS_RCC_GID_PN	0x0121
#define NS_RCC_GA_NXT	0x0100

#define GS_REJECT_RESPONSE_CT_IU 0x8001
#define GS_ACCEPT_RESPONSE_CT_IU 0x8002

/* generic service types */
#define GS_TYPE_ALIAS_SERVICE                   0xf8
#define GS_TYPE_MANAGEMENT_SERVICE              0xfa
#define GS_TYPE_TIME_SERVICE                    0xfb
#define GS_TYPE_DIRECTORY_SERVICE               0xfc

/* service subtypes */
#define SUBTYPE_FABRIC_CONFIGURATION_SERVER	0x01
#define SUBTYPE_NAME_SERVER			0x02
#define SUBTYPE_FABRIC_ZONE_SERVER		0x03
#define SUBTYPE_LOCK_SERVER			0x04
#define SUBTYPE_PERFORMANCE_SERVER		0x05
#define SUBTYPE_SECURITY_POLICY_SERVER		0x06

/* return codes */
#define RC_INVALID_CMND_CODE                    0x1
#define RC_INVALID_VER_LEVEL                    0x2
#define RC_LOGICAL_ERROR                        0x3
#define RC_INVALID_CT_IU_SIZE                   0x4
#define RC_LOGICAL_BUSY                         0x5
#define RC_PROTOCOL_ERROR                       0x7
#define RC_UNABLE_TO_PERFORM_CMND_REQUEST       0x9
#define RC_CMND_NOT_SUPPORTED                   0xb
#define RC_SERVER_NOT_AVAILABLE                 0xd
#define RC_SESSION_COULD_NOT_BE_ESTABLISHED     0xe
#define RC_VENDOR_SPECIFIC_ERROR                0xff

#define RCE_NO_ADDITIONAL_EXPLANATION           0x0
#define RCE_AUTHORIZATION_EXCEPTION             0xf0
#define RCE_AUTHENTICATION_EXCEPTION            0xf1
#define RCE_DATA_BASE_FULL                      0xf2
#define RCE_DATA_BASE_EMPTY                     0xf3
#define RCE_PROCESSING_REQUEST                  0xf4
#define RCE_UNABLE_TO_VERIFY_CONNECTION         0xf5
#define RCE_DEVICES_NOT_IN_A_COMMON_ZONE        0xf6


/* application specific */

#define DEBUG 0x0100
#define VERBOSE 0x0200

#define RET_CODES {					\
        [0x01] = "Invalid command code",		\
        [0x02] = "Invalid version level",		\
        [0x03] = "Logical error",			\
        [0x04] = "Invalid CT_IU size",			\
        [0x05] = "Logical busy",			\
        [0x07] = "Protocol error",			\
        [0x09] = "Unable to perform command request",	\
        [0x0b] = "Command not supported",		\
        [0x0d] = "Server not available",		\
        [0x0e] = "Session could not be established",	\
        [0xff] = "Vendor specific error",		\
}

#define RC_EXPL {						\
        [0x00] = "No additional explanation",			\
	[0x01] = "Port Identifier not registered",		\
	[0x02] = "Port Name not registered",			\
	[0x03] = "Node Name not registered",			\
	[0x04] = "Class of service not registered",		\
	[0x06] = "Initial process associator not registered",	\
	[0x07] = "FC-4 type not registered",			\
	[0x08] = "Symbolic Port Name not registered",		\
	[0x09] = "Symbolic Node Name not registered",		\
	[0x0a] = "Port Type not registered",			\
	[0x0c] = "Fabric Port Name not registered",		\
	[0x0d] = "Hard Address not registered",			\
	[0x0f] = "FC-4 features not registered",		\
	[0x10] = "Access denied",				\
	[0x11] = "Unacceptable Port Identifier",		\
	[0x12] = "Database empty",				\
	[0x13] = "No object registered in the specified scope",	\
	[0x14] = "Domain ID not set",				\
	[0x15] = "Port Number not present",			\
	[0x16] = "No device attached",				\
	[0x30] = "Port List not available",			\
	[0x31] = "Port Type not available",			\
	[0x32] = "Physical Port Number not available",		\
	[0x34] = "Attached Port Name List not available",	\
	[0x36] = "Port State not available",			\
        [0xf0] = "Authorization exception",			\
        [0xf1] = "Authentication exception",			\
        [0xf2] = "DB full",					\
        [0xf3] = "DB empty",					\
        [0xf4] = "Processing request",				\
        [0xf5] = "Unable to verify connection",			\
        [0xf6] = "Device not in a common zone",			\
        [0xff] = ""						\
}

#define PORT_MODULE_TYPE {			\
        [0x01] = "Unknown",			\
        [0x02] = "Other",			\
        [0x03] = "Obsolete",			\
        [0x04] = "Embedded",			\
        [0x05] = "GLM",				\
        [0x06] = "GBIC with serial ID",		\
        [0x07] = "GBIC without serial ID",	\
        [0x08] = "SFP with serial ID",		\
        [0x09] = "SFP without serial ID",	\
        [0x0a] = "XFP",				\
        [0x0b] = "X2 Short",			\
        [0x0c] = "X2 Medium",			\
        [0x0d] = "X2 Tall",			\
        [0x0e] = "XPAK Short",			\
        [0x0f] = "XPAK Medium",			\
        [0x10] = "XPAK TALL",			\
        [0x11] = "XENPAK",			\
        [0x12] = "SFP-DWDM",			\
        [0x13] = "QSFP"				\
}

#define PORT_TX_TYPE {						\
        [0x01] = "Unknown",					\
        [0x02] = "Long wave laser - LL (1550nm)",		\
        [0x03] = "Short wave laser - SN (850nm)",		\
        [0x04] = "Long wave laser cost reduced - LC (1310 nm)",	\
        [0x05] = "Electrical",					\
        [0x06] = "10GBASE-SR 850nm laser",			\
        [0x07] = "10GBASE-LR 1310nm laser",			\
        [0x08] = "10GBASE-ER 1550nm laser",			\
        [0x09] = "10GBASE-LX4 WWDM 1300nm laser",		\
        [0x0a] = "10GBASE-SW 850nm laser",			\
        [0x0b] = "10GBASE-LW 1310nm laser",			\
        [0x0c] = "10GBASE-EW 1550nm laser",			\
}

enum port_type {
        UNIDENTIFIED	= 0x00,
        N_PORT		= 0x01,
        NL_PORT		= 0x02,
        FNL_PORT	= 0x03,
        NX_PORT		= 0x7f,
        F_PORT		= 0x81,
        FL_PORT		= 0x82,
        E_PORT		= 0x84,
        B_PORT		= 0x85,
	NA		= 0xff,
};

#define PORT_TYPE {			\
        [UNIDENTIFIED]	= "Unidentified",\
        [N_PORT]	= "N_Port",	\
        [NL_PORT]	= "NL_Port",	\
        [FNL_PORT]	= "F/NL_Port",	\
        [NX_PORT]	= "Nx_Port",	\
        [F_PORT]	= "F_Port",	\
        [FL_PORT]	= "FL_Port",	\
        [E_PORT]	= "E_Port",	\
        [B_PORT]	= "B_Port",	\
	[NA]		= "N/A",	\
}

#ifndef HBA_PORTSPEED_8GBIT
#define HBA_PORTSPEED_8GBIT 0x00000010
#endif
#ifndef HBA_PORTSPEED_16GBIT
#define HBA_PORTSPEED_16GBIT 0x00000020
#endif
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


#define PORT_SPEED {						\
	[HBA_PORTSPEED_UNKNOWN]	= "Unknown",			\
	[HBA_PORTSPEED_1GBIT]	= "1 GBit/s",			\
	[HBA_PORTSPEED_2GBIT]	= "2 GBit/s",			\
	[HBA_PORTSPEED_10GBIT]	= "10 GBit/s",			\
	[HBA_PORTSPEED_4GBIT]	= "4 GBit/s",			\
	[HBA_PORTSPEED_8GBIT]	= "8 GBit/s",			\
	[HBA_PORTSPEED_16GBIT]	= "16 GBit/s",			\
	[HBA_PORTSPEED_32GBIT]	= "32 GBit/s",			\
	[HBA_PORTSPEED_128GBIT]	= "128 GBit/s",			\
	[HBA_PORTSPEED_64GBIT]	= "64 GBit/s",			\
	[HBA_PORTSPEED_256GBIT]	= "256 GBit/s",			\
	[HBA_PORTSPEED_NOT_NEGOTIATED]	= "not established",	\
}

enum element_type {
	UNKNOWN		= 0x00,
	SWITCH		= 0x01,
	HUB		= 0x02,
	BRIDGE		= 0x03,
};

#define ELEMENT_TYPE {			\
        [UNKNOWN]	= "Unknown",	\
        [SWITCH]	= "Switch",	\
        [HUB]		= "Hub",	\
	[BRIDGE]	= "Brigde",	\
        [NA]		= "N/A",	\
}

enum port_state {
	ONLINE		= 0x01,
	OFFLINE		= 0x02,
	TESTING		= 0x03,
	FAULT		= 0x04,
	VENDOR		= 0xff,
};

#define PORT_STATE {			\
	[UNKNOWN]	= "Unknown",	\
	[ONLINE]	= "Online",	\
	[OFFLINE]	= "Offline",	\
	[TESTING]	= "Testing",	\
	[FAULT]		= "Fault",	\
	[VENDOR]	= "Vendor",	\
}


enum addr_type{
        NPORT = 0x01,
        WWPN  = 0x02,
        BUSID = 0x03,
        FCHOST = 0x04
};

union bus_id {
        unsigned int full;
        struct {
                unsigned char f1;
                unsigned char f2;
                unsigned short dev_id;
        } part;
};

struct ct_iu_preamble {
        char revision;
        char in_id[3];
        char gs_type;
        char gs_subtype;
        char options;
        char res;
        uint16_t code;
        uint16_t size;
        char fragment_id;
        char reason_code;
        char reason_code_exp;
        char vendor_specific;
} __attribute__((__packed__));

struct port_list_entry {
	uint64_t port_name;
	uint8_t port_module_type;
	uint8_t port_tx_type;
	uint8_t port_type;
};

struct interconnect_element {
	uint64_t port_name;
	uint8_t port_type;
};

struct att_port_name {
	uint64_t port_name;
	uint8_t port_flags;
	uint8_t port_type;
};

struct adapter_attr {
	uint32_t handle;
	uint32_t bus_id;
	uint64_t wwpn;
	uint32_t d_id;
	char dev_name[255];
	uint32_t speed;
};

struct ice_conn;

struct ice_conn {
        uint32_t domain_id;
        uint32_t ppn;
        uint64_t port_name;
	uint64_t local_port;
        struct ice_conn *next;
};

static uint16_t display_detail = 0xff;

void print_error(uint8_t rc, uint8_t expl)
{
	char *ret_codes[]  = RET_CODES;
	char *rc_expl[]  = RC_EXPL;

        printf("Error: %s", ret_codes[rc]);
        if (rc ==  RC_UNABLE_TO_PERFORM_CMND_REQUEST)
		printf("-> %s.\n", rc_expl[expl]);
	else
		printf("\n");
}

void print_code(char *c, uint16_t size)
{
	int i;

	for (i = 1; i <= size; i++) {
		printf("%02x ", c[i - 1]);
		if (!(i%8))
			printf("\n");
	}
	printf("\n");
}

struct adapter_attr *fc_get_hba_handle(char *arg)
{
	HBA_STATUS rc;
	HBA_HANDLE hba_handle;
	HBA_ADAPTERATTRIBUTES hba_attr;
	HBA_PORTATTRIBUTES port_attr;
	struct adapter_attr *aa;
	enum addr_type hba_adr_type;
	uint64_t hba_id = 0;
	char *tmp_val;
	int hba_cnt = HBA_GetNumberOfAdapters();
	char hba_name[VLIB_ADAPTERNAME_LEN];
	int cnt;

	if (!hba_cnt)
		return NULL;

	if (strstr(arg, ".")) {
		hba_id = atoi(&arg[0]) << 24;
		hba_id |= atoi(&arg[2]) << 16;
		hba_id |= strtoul(&arg[4], NULL, 16);
		hba_adr_type = BUSID;
	} else if (strstr(arg, "host")) {
		hba_adr_type = FCHOST;
	} else {
		hba_id = strtoull(arg, NULL, 0);
		if (strlen(arg) && !hba_id)
			return NULL;
		if (hba_id & 0xffffffffff000000ULL)
			hba_adr_type = WWPN;
		else
			hba_adr_type = NPORT;
	}

	for (cnt = 0; cnt < hba_cnt; cnt++) {
		if (HBA_GetAdapterName(cnt, hba_name) != HBA_STATUS_OK)
			continue;
		hba_handle = HBA_OpenAdapter(hba_name);
		if (!hba_handle)
			continue;
		rc = HBA_GetAdapterAttributes(hba_handle, &hba_attr) |
		     HBA_GetAdapterPortAttributes(hba_handle, 0, &port_attr);

		if (rc != HBA_STATUS_OK) {
			HBA_CloseAdapter(hba_handle);
			continue;
		}

	       	/* no adapter specifier provided, use first one available */
		if (!hba_id && (hba_adr_type != FCHOST))
			goto out;

		switch(hba_adr_type) {
		case BUSID:
			if (hba_attr.VendorSpecificID == hba_id)
				goto out;
			break;
		case WWPN:
			if (*(uint64_t *)port_attr.PortWWN.wwn == hba_id)
				goto out;
			break;
		case NPORT:
			if (port_attr.PortFcId == hba_id)
				goto out;
			break;
		case FCHOST:
			if (strstr(port_attr.OSDeviceName, arg))
				goto out;
		}
		HBA_CloseAdapter(hba_handle);
	}
	return NULL;
out:
	aa = calloc(1, sizeof(*aa));
	if (!aa)
		return NULL;

	aa->handle = hba_handle;
	aa->bus_id = hba_attr.VendorSpecificID;
	aa->wwpn = *(uint64_t *)port_attr.PortWWN.wwn;
	aa->d_id = port_attr.PortFcId;
	aa->speed = port_attr.PortSpeed;
	strncpy(aa->dev_name, port_attr.OSDeviceName, sizeof(aa->dev_name));

	return aa;
}

char *send_ct_pt(HBA_HANDLE handle, uint32_t req_s, uint32_t resp_s, 
		 uint16_t cmd, char *c_param, uint8_t gs_subtype, 
		 uint8_t gs_type)
{
	HBA_STATUS rc;
	char *req, *resp = NULL;
	struct ct_iu_preamble *p;
	uint32_t i;
	uint16_t code;

        if (posix_memalign((void *)&req, PAGE_SIZE, req_s))
                return NULL;

        if (posix_memalign((void *)&resp, PAGE_SIZE, resp_s))
                goto out_mem;

        memset(req, 0, req_s);
        memset(resp, 0, resp_s);

        p = (struct ct_iu_preamble *) req;

        p->revision = 0x03;
        p->gs_type = gs_type;
        p->gs_subtype = gs_subtype;
	p->code = cmd;
	p->size = (resp_s - CT_IU_PREAMBLE_SIZE) >> 2;

	memcpy(req + CT_IU_PREAMBLE_SIZE, c_param, req_s - CT_IU_PREAMBLE_SIZE);

        rc = HBA_SendCTPassThru(handle, req, req_s, resp, resp_s);

	if (display_detail & DEBUG) {
		printf("--- REQUEST cmd = 0x%04x ---\n", cmd);
		print_code(req, req_s);
		printf("--- RESPONSE rc = 0x%x ---\n", rc);
		print_code(resp, resp_s);
	}
	if (rc) {
		free(resp);
		resp = NULL;
        }  
out_mem:
	free(req);
	return resp;
}

void print_error_statement(void)
{
	printf("\n*** Warning: Received \"non-conforming\" data from management server. ***\n");
	printf("***          Contact your switch supplier for support.              ***\n");
}
