/*
 * zfcp_show
 *
 * Fibre Channel querying utility.
 *
 * Copyright IBM Corp. 2009, 2010.
 *
 *  Author(s): Swen Schillig <swen@vnet.ibm.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <hbaapi.h>
#include "include/zfcp_util.h"

#define PL_SIZE(x) 4 + x * 12

#define TOPOLOGY	0x0400
#define ATTACHMENT	0x0800
#define DOMAIN		0x1000
#define CSV		0x2000

char *send_ct(HBA_HANDLE handle, uint32_t req_s, uint32_t resp_s,
              uint16_t cmd, char *c_param, uint8_t gs_subtype, uint8_t gs_type)
{
	char *resp;
	uint16_t code;
	uint8_t retries = 3;

	while (retries--) {
		resp = send_ct_pt(handle, req_s, resp_s, cmd, c_param, 
				  gs_subtype, gs_type);
		if (!resp) {
			sleep(1);
			continue;
		}

		code = *(uint16_t *)(resp + CT_IU_CODE_OFFSET);

		switch (code) {
		case GS_ACCEPT_RESPONSE_CT_IU:
			return resp;
		case GS_REJECT_RESPONSE_CT_IU:
			break;
		default:
			/* non-conforming return code */
			if (!retries)
				print_error_statement();
			else
				sleep(1);
		} 
		free(resp);
	}
	return NULL;
}

void show_ns_info(HBA_HANDLE handle, uint64_t value)
{
	char *resp, *payload;
	uint32_t tmp = 0, port_id = 0;
	uint64_t port_name = 0;
	uint32_t proto;
	char *pt[] = PORT_TYPE;
	char prot_str[255];

	printf("\nLocal Port List:\n");

	do {
		resp = send_ct(handle, CT_IU_PREAMBLE_SIZE + 4,
			       CT_IU_PREAMBLE_SIZE + 640,
			       NS_RCC_GA_NXT, (char *)&port_id,
			       SUBTYPE_NAME_SERVER, GS_TYPE_DIRECTORY_SERVICE);
	
		if (!resp)
			return;

		memset(prot_str, 0, 255);

		payload = resp + CT_IU_PREAMBLE_SIZE;

		if (!tmp)
			tmp = port_id;

		port_id = (*(uint32_t *) (payload + 1)) >> 8;
		port_name = (*(uint64_t *) (payload + 4));
		proto = (*(uint32_t *) (payload + 560));

		if (tmp == port_id)
			break;
		if ((display_detail & ATTACHMENT) &&
		    !(value == port_id || value == port_name))
			continue;

		printf("\t 0x%016lx / 0x%x [%s] ", port_name, port_id, 
			pt[*payload]);

		if (proto & 0x100)
			strcat(prot_str, " SCSI-FCP ");
		if ((proto & 0x18000000) == 0x18000000)
			strcat(prot_str, " FICON ");

		if (strlen(prot_str))
			printf("proto =%s\n", prot_str);
		else
			printf("\n");

		free(resp);
	} while (1);
}

uint32_t get_ice_list(HBA_HANDLE handle, struct interconnect_element **p_ice)
{
	struct interconnect_element *ice = NULL;
	char *resp, *payload;
	uint16_t i, offset;
	uint32_t entries = 0;

	resp = send_ct(handle, 
		       CT_IU_PREAMBLE_SIZE, 
		       CT_IU_PREAMBLE_SIZE + PL_SIZE(100), 
		       FCS_RCC_GIEL, NULL, 
		       SUBTYPE_FABRIC_CONFIGURATION_SERVER, 
		       GS_TYPE_MANAGEMENT_SERVICE);

	if (resp) {
		payload = resp + CT_IU_PREAMBLE_SIZE; 
		entries = *(uint32_t *)payload;
		ice = calloc(entries, sizeof(struct interconnect_element));
		if (!ice) 
			entries = 0;
	}

	for (i = 0, offset = 4; i < entries; i++, offset++) {
		ice[i].port_name = *(uint64_t *)(payload + offset);
		offset += 11;
		ice[i].port_type = *(payload + offset);
	}

	*p_ice = ice;
	free(resp);
	return entries;
}

uint32_t get_port_list(HBA_HANDLE handle, uint64_t const ice_name, struct port_list_entry **pple)
{
	struct port_list_entry *ple = NULL;
	char *resp, *payload;
	uint16_t i, offset = 4;
	uint32_t entries = 0;

	resp = send_ct(handle,
		       CT_IU_PREAMBLE_SIZE + sizeof(ice_name), 
		       CT_IU_PREAMBLE_SIZE + PL_SIZE(500), 
		       FCS_RCC_GPL, (char *)&ice_name, 
		       SUBTYPE_FABRIC_CONFIGURATION_SERVER,
		       GS_TYPE_MANAGEMENT_SERVICE);

	if (resp) {
		payload = resp + CT_IU_PREAMBLE_SIZE; 
		entries = *(uint32_t *)payload; 
		ple = calloc(entries, sizeof(*ple));
		if (!ple)
			entries = 0;
	}

	for (i = 0; i < entries; i++) {
		ple[i].port_name = *(uint64_t *) (payload + offset);
		offset += 9;
		ple[i].port_module_type = *(payload + offset++);
		ple[i].port_tx_type = *(payload + offset++);
		ple[i].port_type = *(payload + offset++);
	}

	*pple = ple;
	free(resp);
	return entries;
}

uint32_t get_att_port_list(HBA_HANDLE handle, uint64_t const icep_name, 
			   struct att_port_name **papn)
{
	struct att_port_name *apn = NULL;
	char *resp, *payload;
	uint32_t entries = 0, offset = 4;
	uint16_t i;

	resp = send_ct(handle,
		       CT_IU_PREAMBLE_SIZE + sizeof(icep_name), 
		       CT_IU_PREAMBLE_SIZE + PL_SIZE(100),
		       FCS_RCC_GAPNL, (char *)&icep_name,
		       SUBTYPE_FABRIC_CONFIGURATION_SERVER,
		       GS_TYPE_MANAGEMENT_SERVICE);

	if (resp) {
		payload = resp + CT_IU_PREAMBLE_SIZE;
		entries = *(uint32_t *)payload;
		apn = calloc(entries, sizeof(*apn));
		if (!apn)
			entries = 0;
	}

	for (i = 0; i < entries; i++) {
		apn[i].port_name = *(uint64_t *)(payload + offset);
		offset += 10;
		apn[i].port_flags = *(payload + offset++); 
		apn[i].port_type  = *(payload + offset++);
	}

	*papn = apn;
	free(resp);
	return entries;
}

uint8_t get_domain_id(HBA_HANDLE handle, uint64_t icep_name)
{
	char *resp;
	uint16_t domain_id;

	resp = send_ct(handle,
		       CT_IU_PREAMBLE_SIZE + sizeof(icep_name),
		       CT_IU_PREAMBLE_SIZE + 4,
		       FCS_RCC_GDID, (char *) &icep_name,
		       SUBTYPE_FABRIC_CONFIGURATION_SERVER,
		       GS_TYPE_MANAGEMENT_SERVICE);
	if (!resp)
		return 0;

	domain_id = *(uint16_t *)(resp + CT_IU_PREAMBLE_SIZE);

	free(resp);
	return domain_id & 0xff;
}


uint8_t get_port_state(HBA_HANDLE handle, uint64_t port_name)
{
	char *resp;
	uint16_t port_state;

	resp = send_ct(handle,
		       CT_IU_PREAMBLE_SIZE + sizeof(port_name),
		       CT_IU_PREAMBLE_SIZE + 8,
		       FCS_RCC_GPS, (char *)&port_name,
		       SUBTYPE_FABRIC_CONFIGURATION_SERVER,
		       GS_TYPE_MANAGEMENT_SERVICE);
	if (!resp)
		return 0;

	port_state = *(uint8_t *)(resp + CT_IU_PREAMBLE_SIZE + 7);

	free(resp);
	return port_state;
}

uint32_t get_ppn(HBA_HANDLE handle, uint64_t port_name)
{
	char *resp;
	uint32_t ppn = 0xFFFFFFFF;

	resp = send_ct(handle,
		       CT_IU_PREAMBLE_SIZE + sizeof(port_name),
		       CT_IU_PREAMBLE_SIZE + 4,
		       FCS_RCC_GPPN, (char *) &port_name,
		       SUBTYPE_FABRIC_CONFIGURATION_SERVER,
		       GS_TYPE_MANAGEMENT_SERVICE);
	if (!resp)
		return ppn;

	ppn = *(uint32_t *) (resp + CT_IU_PREAMBLE_SIZE);

        free(resp);
        return ppn;
}

char * get_information_list(HBA_HANDLE handle, uint64_t name)
{
	char *resp, *payload, *ret_str = NULL;
	uint16_t offset = 3;
	uint8_t length, i;

	resp = send_ct(handle,
		       CT_IU_PREAMBLE_SIZE + sizeof(name),
		       CT_IU_PREAMBLE_SIZE + 256,
		       FCS_RCC_GIEIL, (char *) &name,
		       SUBTYPE_FABRIC_CONFIGURATION_SERVER,
		       GS_TYPE_MANAGEMENT_SERVICE);
	if (!resp) 
		return NULL;

	payload = resp + CT_IU_PREAMBLE_SIZE;
	length = *(payload + offset++);
	ret_str = calloc(length + 1, 1);
	if (ret_str)
		memcpy(ret_str, payload + offset, length);

        free(resp);
        return ret_str;
}


char * get_ice_logical_name(HBA_HANDLE handle, uint64_t name)
{
	char *resp, *payload, *ret_str = NULL;

	resp = send_ct(handle,
		       CT_IU_PREAMBLE_SIZE + sizeof(name),
		       CT_IU_PREAMBLE_SIZE + 256,
		       FCS_RCC_GIELN, (char *) &name,
		       SUBTYPE_FABRIC_CONFIGURATION_SERVER,
		       GS_TYPE_MANAGEMENT_SERVICE);
	if (!resp)
		return NULL;

	payload = resp + CT_IU_PREAMBLE_SIZE;
	ret_str = calloc(*(uint8_t *)payload + 1, 1);
	if (ret_str)
		strncpy(ret_str, payload + 1 , *(uint8_t *)payload);

        free(resp);
        return ret_str;
}


uint32_t get_destination_id(HBA_HANDLE handle, uint64_t port_name)
{
        char *resp;
	uint32_t d_id;

        resp = send_ct(handle,
		       CT_IU_PREAMBLE_SIZE + sizeof(port_name),
		       CT_IU_PREAMBLE_SIZE + 4,
		       NS_RCC_GID_PN, (char *) &port_name,
		       SUBTYPE_NAME_SERVER, GS_TYPE_MANAGEMENT_SERVICE);
        if (!resp)
                return 0;

        d_id = *(uint32_t *)(resp + CT_IU_PREAMBLE_SIZE);

        free(resp);
        return d_id & 0xFFFFFF;
}

void print_topology(struct ice_conn *ic)
{
	struct ice_conn *ic_cur, *tmp;
	uint8_t domain_id = 0xff;

	for (ic_cur = ic; ic_cur; ic_cur = ic_cur->next) {
		for (tmp = ic; tmp; tmp = tmp->next) 
			if (tmp->local_port == ic_cur->port_name)
				break;

		if (!tmp)
			continue;

		if (domain_id != ic_cur->domain_id)
			printf("\nDomain %03d attached via\n", ic_cur->domain_id);
		
		printf("\tphysical port %03d to physical port %03d of domain %d\n", 
			ic_cur->ppn, tmp->ppn, tmp->domain_id);

		domain_id = ic_cur->domain_id;
	}
}

void print_ice_info(HBA_HANDLE handle, struct interconnect_element *ice, 
		    uint8_t domain_id, uint32_t pl_entries)
{
	char *element_type[] = ELEMENT_TYPE;
	char *ice_info, *tmp;

	printf("\n");
	printf("Interconnect Element Name       0x%lx\n", ice->port_name);
	printf("Interconnect Element Domain ID  %03d\n", domain_id);
	printf("Interconnect Element Type       %s\n", element_type[ice->port_type]);
	printf("Interconnect Element Ports      %03d\n", pl_entries);	

	if (!(display_detail & VERBOSE))
		return;

	ice_info = get_information_list(handle, ice->port_name);
	if (!ice_info) {
		printf("Interconnect Element Vendor     Error\n");
		printf("Interconnect Element Model      Error\n");
		printf("Interconnect Element Rel. Code  Error\n");
	} else {
		printf("Interconnect Element Vendor     %s\n", ice_info);
		tmp = index(ice_info, 0) + 1;
		printf("Interconnect Element Model      %s\n", tmp);
		tmp = index(tmp, 0) + 1;
		printf("Interconnect Element Rel. Code  %s\n", tmp);
		free(ice_info);
	}

	tmp = get_ice_logical_name(handle, ice->port_name);
	if (!tmp)
		printf("Interconnect Element Log. Name  Error\n");
	else {
		printf("Interconnect Element Log. Name  %s\n", tmp);
		free(tmp);
	}
}

void print_csv(struct interconnect_element *ice, uint8_t domain_id,
	       struct port_list_entry *ple, uint8_t ps, uint32_t ppn,
	       struct att_port_name *apn, uint32_t d_id)
{
	char *element_type[]	= ELEMENT_TYPE;
	char *port_state[]	= PORT_STATE;
	char *port_tx_type[]	= PORT_TX_TYPE;
	char *port_type[]	= PORT_TYPE;
	char *port_module_type[] = PORT_MODULE_TYPE;

	printf("0x%016lx,%03d,%s,%03d,%s,0x%016lx,%s,%s,%s,",
		ice->port_name, domain_id, element_type[ice->port_type],
		ppn, port_state[ps], ple->port_name, 
		port_module_type[ple->port_module_type],
		port_tx_type[ple->port_tx_type],
		port_type[ple->port_type]);

	if (ps == ONLINE && apn)
		printf("0x%016lx,0x%06x,%s\n", apn->port_name, d_id,
			port_type[apn->port_type]);
	else
		printf("n/a,n/a,n/a\n");

}
		
void print_ple_info(struct port_list_entry *ple, uint8_t ps, uint32_t ppn)
{
	char *port_state[]   = PORT_STATE;
	char *port_tx_type[] = PORT_TX_TYPE;
	char *port_type[]    = PORT_TYPE;
	char *port_module_type[] = PORT_MODULE_TYPE;

	if (display_detail & VERBOSE) {
		printf("\n\tICE Port %03d  %s [0x%lx]\n", ppn, 
			port_state[ps], ple->port_name);
		if (ps != OFFLINE)
			printf("\tICE Port Type %s %s [%s]\n", 
				port_module_type[ple->port_module_type],
				port_tx_type[ple->port_tx_type],
				port_type[ple->port_type]);
	} else
		printf("\tICE Port %03d  %s\n", ppn, port_state[ps]);

}

struct ice_conn *gen_topology(struct ice_conn *icc_start, uint8_t domain_id, 
			      uint32_t ppn, struct att_port_name *apn, 
			      uint8_t *inc, struct port_list_entry *ple)
{
	struct ice_conn *icc_new, *icc_cur = icc_start;

	if (ple->port_type != E_PORT) 
		return icc_start;

	icc_new = calloc(1, sizeof(struct ice_conn));

	if (!icc_new) {
		if (display_detail & DEBUG)
			printf("Error: failed to allocate memory.\n");
		(*inc) |= 1; /* mark as incomplete */
		return NULL;
	}

	if (!icc_start)
		icc_cur = icc_new;
	else {
		while(icc_cur->next)
			icc_cur = icc_cur->next;
		icc_cur->next = icc_new;
	}
		
	icc_new->domain_id = domain_id;
	icc_new->ppn = ppn;
	icc_new->port_name = apn->port_name;
	icc_new->local_port = ple->port_name;
	
	return icc_start ? icc_start : icc_cur;
}

void print_apn_info(struct att_port_name *apn, uint32_t d_id)
{
	char *port_type[] = PORT_TYPE;

	printf("\t\tAttached Port [WWPN/ID] 0x%lx / 0x%06x [%s]\n", 
		apn->port_name, d_id, port_type[apn->port_type]);
}

void free_topology_mem(struct ice_conn *p)
{
	struct ice_conn *tmp;

	while (p) {
		tmp = p;
		p = tmp->next;
		free(tmp);
	}
}

HBA_STATUS do_something(struct adapter_attr *aa, uint64_t value)
{
	struct interconnect_element *ice;
	struct port_list_entry *ple;
	struct att_port_name *apn;
	struct ice_conn *icc_start = NULL;
	uint32_t ice_no, apn_no, ple_no;
	uint32_t ice_cnt, apn_cnt, ple_cnt;
	uint32_t ppn, d_id;
	uint8_t domain_id, ps, incompl = 0;

	ice_no = get_ice_list(aa->handle, &ice);
	if (!ice_no) {
		printf("ERROR: no interconnect elements found.\n");
		return 1;
	}

	if (display_detail & CSV)
		printf("ICE-name,domain,ICE-type,ppn,status,port name,"
		       "port module type,port TX type,port type,"
		       "att. port name,att. port ID,att. port type\n");

	for (ice_cnt = 0; ice_cnt < ice_no; ice_cnt++) {
		domain_id = get_domain_id(aa->handle, ice[ice_cnt].port_name);
		if (!domain_id)
			incompl |= 1;

		if ((display_detail & DOMAIN) && (value != domain_id))
			continue;

		ple_no = get_port_list(aa->handle, ice[ice_cnt].port_name, &ple);

		if (!(display_detail & (ATTACHMENT | TOPOLOGY | CSV)))
			print_ice_info(aa->handle, &ice[ice_cnt], domain_id, ple_no);

		if (!ple_no)
			incompl |= 1;

		for (ple_cnt = 0; ple_cnt < ple_no; ple_cnt++) {
			ps = get_port_state(aa->handle, ple[ple_cnt].port_name);

			if (!(display_detail & ps))
				continue;

			ppn = get_ppn(aa->handle, ple[ple_cnt].port_name);

			if (!(display_detail & (ATTACHMENT | TOPOLOGY | CSV)))
				print_ple_info(&ple[ple_cnt], ps, ppn);

			if (ps != ONLINE) {
				if (display_detail & CSV)
					print_csv(&ice[ice_cnt], domain_id, 
						  &ple[ple_cnt], ps, ppn, NULL,
						  0);
				continue;
			}

			apn_no = get_att_port_list(aa->handle, 
						   ple[ple_cnt].port_name, 
						   &apn);
			if (!apn_no)
				incompl |= 1;

			for (apn_cnt = 0; apn_cnt < apn_no; apn_cnt++) {
				d_id = get_destination_id(aa->handle,
							  apn[apn_cnt].port_name);

				if ((display_detail & ATTACHMENT) &&
				    (value == d_id || 
				     value == apn[apn_cnt].port_name)) {
					print_ice_info(aa->handle, &ice[ice_cnt], 
						       domain_id, ple_no);
					print_ple_info(&ple[ple_cnt], ps, ppn);
					print_apn_info(&apn[apn_cnt], d_id);
				}
				if (!(display_detail & (ATTACHMENT | TOPOLOGY | CSV)))
					print_apn_info(&apn[apn_cnt], d_id);

				if (display_detail & CSV)
					print_csv(&ice[ice_cnt], domain_id,
						  &ple[ple_cnt], ps, ppn,
						  &apn[apn_cnt], d_id);
			}

			if (display_detail & (TOPOLOGY | VERBOSE)) 
				icc_start = gen_topology(icc_start, domain_id, 
							 ppn, apn, &incompl,
							 &ple[ple_cnt]);
			free(apn);
		} 
		free(ple);
	} 
	free(ice);

	if (display_detail & (TOPOLOGY | VERBOSE))
		if (icc_start) {
			printf("\n\t*** Topology ***\n");
			print_topology(icc_start);
			free_topology_mem(icc_start);
		} else 
			printf("\n\t*** No topology information available ***\n");

	if (incompl)
		printf("\n*** Warning: at least one command did not succeed. ***\n"
		       "*** Data might be incomplete.                      ***\n");

	return 0;
}

void print_usage(void)
{
        printf("Usage: zfcp_show [-oOtnvcdh] [-a <busid|fc_host|S_ID|WWPN>]\n");
        printf("                 [-i <DOMAIN-ID>] [-p <attached port>]\n");
        printf("\t-a: source adapter specified by busid, fc_host, S_ID or WWPN.\n");
        printf("\t-o: show online ports only.\n");
        printf("\t-O: show offline ports only.\n");
        printf("\t-i: show domain <ID> only.\n");
        printf("\t-p: show attached port <WWPN|ID> only.\n");
        printf("\t-t: show topology information only.\n");
        printf("\t-n: show local name server information.\n");
        printf("\t-c: provide the result as CSV output.\n");
        printf("\t-d: provide some debug output.\n");
        printf("\t-v: be verbose.\n");
        printf("\t-h: this help text.\n");
}

int main(int argc, char *argv[]) {

        HBA_STATUS rc;
	struct adapter_attr *aa;
        char adapter[256] = "\0", dest[256] = "\0";
	char *speed[] = PORT_SPEED;
	uint64_t value = 0;
	int arg, ns_only = 0;
	int cc = 0;

        rc = HBA_LoadLibrary();

        if (rc != HBA_STATUS_OK) {
                printf("Failed to load library.\n");
                return rc;
        }

        while ((arg = getopt(argc, argv, "a:p:i:vdtchoOn")) != -1) {
		cc++;
                switch (arg) {
                case 'a':
                        strncpy(adapter, optarg, 255);
                        break;
                case 'v':
			display_detail |= VERBOSE;
                        break;
                case 'd':
			display_detail |= DEBUG;
                        break;
		case 't':
			if (display_detail & (DOMAIN | ATTACHMENT)) {
				printf("No limiting filter allowed in conjunction "
				       "with topology output.\n");
				goto end;
			}
			display_detail |= TOPOLOGY;
			break;
		case 'i':
			if (value) {
				printf("Only one filter is allowed at a time.\n");
				goto end;
			}
			if (display_detail & TOPOLOGY) {
				printf("No limiting filter allowed in conjunction "
				       "with topology output.\n");
				goto end;
			}
			display_detail |= DOMAIN;
			value = strtoull(optarg, NULL, 0);
			if (strlen(optarg) && !value) {
				printf("Invalid value.\n");
				goto end;
			}
			break;
		case 'p':
			if (value) {
				printf("Only one filter is allowed at a time.\n");
				goto end;
			}
			if (display_detail & TOPOLOGY) {
				printf("No limiting filter allowed in conjunction "
				       "with topology output.\n");
				goto end;
			}
			display_detail |= ATTACHMENT;
			value = strtoull(optarg, NULL, 0);
			if (strlen(optarg) && !value) {
				printf("Invalid value.\n");
				goto end;
			}
			break;			
		case 'c':
			display_detail |= CSV;
			break;
		case 'o':
			display_detail ^= 0xff ^ ONLINE;
			break;
		case 'O':
			display_detail ^= 0xff ^ OFFLINE;
			break;
		case 'n':
			ns_only = 1;
			break;	
                case 'h':
                        print_usage();
                        exit(0);
                default:
                        printf("Unknown parameter '-%c'\n", arg);
                        print_usage();
                        goto end;
                }
        }

	if ((display_detail & CSV)  && (cc > 1)) {
		printf("No additional switches allowed in conjunction with CVS (-c) output.\n");
		goto end;
	}
        aa = fc_get_hba_handle(adapter);

        if (aa) {
		if (display_detail & VERBOSE)
			printf("Using adapter BUS_ID    0.0.%x\n"
			       "              Name      0x%016llx\n" 
			       "              N_Port_ID 0x%x\n" 
			       "              OS-Device %s\n"
			       "              Speed     %s\n",
        	               aa->bus_id, aa->wwpn, aa->d_id, aa->dev_name, 
			       speed[aa->speed]);

		if (!ns_only)
                	rc = do_something(aa, value);
		else
			show_ns_info(aa->handle, value);

                HBA_CloseAdapter(aa->handle);
        } else {
		printf("No adapter found.\n");
		rc = 1;
	}

	
end:
        HBA_FreeLibrary();
        return rc;
}

