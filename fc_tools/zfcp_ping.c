/*
 * zfcp_ping
 *
 * Fibre Channel ping.
 *
 * Copyright IBM Corp. 2009, 2010.
 *
 * Author(s): Swen Schillig <swen@vnet.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <hbaapi.h>
#include <errno.h>
#include "include/zfcp_util.h"

#define FC_PNG_REV              0x00000001

static struct {
	unsigned int min;
	double avg;
	unsigned int max;
	unsigned int count;
} time_stat;

struct fpng {
	uint32_t revision;
	uint16_t tag;
	uint16_t length;
	union {
		struct {
			uint64_t pval;
			uint32_t token;
		} name;
		struct {
			uint32_t pval;
			uint32_t token;
		} id;
	} t;
} __attribute__((__packed__));

unsigned int calc_time_diff(struct timeval *start, struct timeval *end)
{
        return (end->tv_sec - start->tv_sec) * 1000000 +
               (end->tv_usec - start->tv_usec);
}

double update_ts(struct timeval *start, struct timeval *end)
{
	unsigned int diff = calc_time_diff(start, end);

	if (time_stat.count == 0) {
		time_stat.min = diff;
		time_stat.avg = diff;
		time_stat.max = diff;
		time_stat.count++;
	} else {
		if (time_stat.min > diff)
			time_stat.min = diff;
		if (time_stat.max < diff)
			time_stat.max = diff;
		time_stat.avg += (diff - time_stat.avg) / ++time_stat.count;
	}       
	return diff / 1000.0;
}

static void timing_results(int sig)
{
	printf("\n---------- ping statistics -----------\n");
	printf("min/avg/max = %0.03f/%0.03f/%0.03f ms",
	       time_stat.min / 1000.0,
	       time_stat.avg / 1000.0,
	       time_stat.max / 1000.0);
	printf("\n--------------------------------------\n");
	
	if (sig != SIGHUP)
		exit(0);
}

void setup_signal_handling(void)
{
	struct sigaction action;

	memset(&action, 0, sizeof(action));
	action.sa_handler = timing_results;
	action.sa_flags = 0;

	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGQUIT, &action, NULL);
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGHUP, &action, NULL);
}

HBA_STATUS send_fc_ping(struct adapter_attr *aa, char *dest, unsigned int token, 
			unsigned int count)
{
	struct ct_iu_preamble *pre;
	char *resp;
	char *speed[] = PORT_SPEED;
	struct timeval start, end;
	struct fpng p;
	uint32_t *tp;
	uint64_t dest_val = strtoull(dest, NULL, 0);
	uint8_t retry = 2;
	HBA_UINT32 size;
	HBA_STATUS rc = HBA_STATUS_OK;

	if (!dest_val)
		return HBA_STATUS_ERROR_ARG;

	bzero(&p, 20);

	p.revision = FC_PNG_REV;

	if (dest_val & 0xFFFFFFFFFF000000ULL) {
		p.tag = FC_TAG_WWPN;
		p.length = 8;
		p.t.name.pval = dest_val;
		p.t.name.token = token;
		tp = &p.t.name.token;
                size = CT_IU_PREAMBLE_SIZE + 20;
        } else {
		p.tag = FC_TAG_NPORT;
		p.length = 4;
		p.t.id.pval = (dest_val & 0xFFFFFF) << 8;
		p.t.id.token = token;
		tp = &p.t.id.token;
                size = CT_IU_PREAMBLE_SIZE + 16;
        }

	if (display_detail & VERBOSE)
		printf("Sending PNG from BUS_ID=0.0.%x WWPN=0x%016llx "
		       "ID=0x%x dev=%s speed=%s\n", aa->bus_id, aa->wwpn, 
		       aa->d_id, aa->dev_name, speed[aa->speed]);
	else
		printf("Sending PNG from BUS_ID=0.0.%x speed=%s\n", 
		       aa->bus_id, speed[aa->speed]);

	while (count) {
		gettimeofday(&start, NULL);
		resp = send_ct_pt(aa->handle, size, CT_IU_PREAMBLE_SIZE + 4, 
				  FCS_RCC_FPNG, (char *)&p, 
				  SUBTYPE_FABRIC_CONFIGURATION_SERVER,
				  GS_TYPE_MANAGEMENT_SERVICE);
		gettimeofday(&end, NULL);

		if (!resp) { 
			if (retry--)
				continue;
			else 
				return HBA_STATUS_ERROR;
		} else
			retry = 2;

		pre = (struct ct_iu_preamble *)resp;

		switch (pre->code) {
		case GS_REJECT_RESPONSE_CT_IU:
			if ((pre->reason_code == RC_LOGICAL_ERROR) &&
			    (pre->reason_code_exp == RCE_PROCESSING_REQUEST)) {
				if (display_detail & VERBOSE)
					printf("Warning: Token %d in use. " 
					       "Incrementing.\n", *tp);
				(*tp)++;
			} else {
				if (display_detail & VERBOSE)
					print_error(pre->reason_code,
						    pre->reason_code_exp);
				goto err_out;
			}
			break;
		case GS_ACCEPT_RESPONSE_CT_IU: 
			if (p.tag == FC_TAG_WWPN)
				printf("\techo received from WWPN (0x%016lx) "
				       "tok=%d time=%0.03f ms\n", dest_val, 
				       (*tp)++, update_ts(&start, &end));
			else
				printf("\techo received from D_ID (0x%x) "
				       "tok=%d time=%0.03f ms\n", dest_val,
				       (*tp)++, update_ts(&start, &end));
			sleep(1);
			count--;
			break;
		default:
			print_error_statement();
			goto err_out;
		}
		free(resp);
	}

	return HBA_STATUS_OK;
err_out:
	free(resp);
	return HBA_STATUS_ERROR;
}

void print_usage(void)
{
	printf("Usage: zfcp_ping [-vdh] [-c <count> ] [-t <token>] -a <busid|fc_host|S_ID|WWPN> <WWPN|D_ID>\n");
	printf("\t-a: source adapter specified by busid, fc_host, S_ID or WWPN.\n");
	printf("\t-c: number of ping requests to send.\n");
	printf("\t-t: token to send by ping request.\n");
	printf("\t-v: be verbose.\n");
	printf("\t-d: provide some debug output.\n");
	printf("\t-h: this help text.\n");
}

int main(int argc, char *argv[]) {

	struct adapter_attr *aa;
	int arg, rc = HBA_STATUS_OK;
	uint32_t token = 0, cnt = 3;
	char adapter[256] = "\0", dest[256] = "\0";

	while ((arg = getopt(argc, argv, "a:c:t:vdh")) != -1) {
		switch (arg) {
		case 'a':
			strncpy(adapter, optarg, 255);
			break;
		case 'c':
			cnt = strtoul(optarg, NULL, 0);
			if ((strlen(optarg) && !cnt) || (optarg[0] == '-')) {
				printf("Invalid value for count.\n");
				return HBA_STATUS_ERROR;
			}
			break;
		case 't':
			token = (uint32_t) strtoul(optarg, NULL, 0);
			if ((strlen(optarg) && !token) || (optarg[0] == '-')) {
				printf("Invalid value.\n");
				return HBA_STATUS_ERROR;
			}
			break;
		case 'v':
                        display_detail |= VERBOSE;
                        break;
                case 'd':
                        display_detail |= DEBUG;
                        break;
		case 'h':
			print_usage();
			exit(0);
		default:
			printf("Unknown parameter '-%c'\n", arg);
			print_usage();
			return HBA_STATUS_ERROR;
		}
	}

	if ((optind + 1) == argc)
		strncpy(dest, argv[optind], 255);
	else {
		printf("Invalid parameter.\n");
		print_usage();
		return HBA_STATUS_ERROR;
	}

	if (HBA_LoadLibrary() != HBA_STATUS_OK) {
		printf("Error: Failed to load library.\n");
		return HBA_STATUS_ERROR;
	}

	aa = fc_get_hba_handle(adapter);
	if (!aa) {
		printf("No adapter found.\n");
		HBA_FreeLibrary();
		return HBA_STATUS_ERROR;
	}

	time_stat.count = 0;
	setup_signal_handling();

	if (send_fc_ping(aa, dest, token, cnt) != HBA_STATUS_OK) {
		printf("Error received for FPNG request, aborting.\n");
		rc = HBA_STATUS_ERROR;
	}

	HBA_CloseAdapter(aa->handle);
	free(aa);

	timing_results(0);		

	HBA_FreeLibrary();
	return rc;
}

