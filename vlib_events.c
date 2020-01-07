/*
 * Copyright IBM Corp. 2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:		Sven Schuetz <sven@de.ibm.com>
 *
 * File:		vlib_events.c
 *
 * Description:
 * This file contains all event handling functions
 *
 */

#include "vlib.h"

#define SCSITRANSPORT_MSG_SIZE (sizeof(struct fc_nl_event) + \
			       sizeof(struct nlmsghdr))
#define MAX_SLOTS 10

/* note: the event * returned by that function can only be used as long as
 * the mutex is held, as it can be overwritten as soon as the mutex is
 * returned from the caller */
struct vlib_event *popEvent(struct vlib_adapter *adapter)
{
	struct vlib_event *event;

	event = adapter->event_queue.first;

	/* event list is empty */
	if (!event)
		return NULL;

	/* move event from the event queue to the free event list so that it can
	 * be reused for new incoming events as soon as the mutex is returned */
	adapter->event_queue.first = event->next;
	if (adapter->event_queue.first == NULL)
		/* if list is empty make sure the last element is NULL as well*/
		adapter->event_queue.last = NULL;

	/* does the free list have at least one member? */
	if (adapter->free_event_list.last)
		/* yes, just append */
		adapter->free_event_list.last->next = event;
	else {
		/* no, make this the first element */
		adapter->free_event_list.first = event;
		adapter->free_event_list.first->next = NULL;
	}
	/* either way - the appended event is always the last */
	adapter->free_event_list.last = event;

	return event;
}

static void appendEvent(struct vlib_event *newEvent,
						struct vlib_adapter *adapter)
{
	struct vlib_event *event;

	/* first check if we have a free event available in the free list */
	if (adapter->free_event_list.first) {
		event = adapter->free_event_list.first;
		adapter->free_event_list.first = event->next;
		/* if list is empty make sure the last element is NULL as well*/
		if (adapter->free_event_list.first == NULL)
			adapter->free_event_list.last = NULL;
	}
	/* if not, overwrite the oldest event in the event list */
	else {
		event = adapter->event_queue.first;
		adapter->event_queue.first = event->next;
	}

	memcpy(event, newEvent, sizeof(struct vlib_event));
	event->next = NULL;

	/* there are already events in the list */
	if (adapter->event_queue.last) {
		adapter->event_queue.last->next = event;
		adapter->event_queue.last = event;
	}
	/* it's the first event to be added to the list */
	else {
		adapter->event_queue.last = event;
		adapter->event_queue.first = event;
	}

	return;
}

void free_event_queue(struct vlib_adapter *adapter)
{
	struct vlib_event *event;

	while (event = adapter->event_queue.first) {
		adapter->event_queue.first = event->next;
		free(event);
	}

	while (event = adapter->free_event_list.first) {
		adapter->free_event_list.first = event->next;
		free(event);
	}
}

void init_event_queue(struct vlib_adapter *adapter)
{
	int i;
	struct vlib_event *event;

	adapter->event_queue.first = NULL;
	adapter->event_queue.slots_used = 0;

	adapter->free_event_list.first = calloc(1, sizeof(struct vlib_event));
	if (adapter->free_event_list.first == NULL) {
		VLIB_PERROR(ENOMEM, "ERROR");
		return;
	}

	adapter->free_event_list.last = adapter->free_event_list.first;

	for (i = 0; i < MAX_SLOTS - 1; i++) {
		event = calloc(1, sizeof(struct vlib_event));
		if (event == NULL) {
			VLIB_PERROR(ENOMEM, "ERROR");
			return;
		}
		adapter->free_event_list.last->next = event;
		adapter->free_event_list.last = event;
	}
	adapter->free_event_list.slots_used = MAX_SLOTS;
}

static void process_event(struct fc_nl_event *fc_nle)
{
	struct vlib_event event;
	HBA_EVENTINFO *hba_event;
	struct vlib_adapter *adapter;

	VLIB_MUTEX_LOCK(&vlib_data.mutex);

	adapter = getAdapterByHostNo(fc_nle->host_no);
	if (!adapter || adapter->handle == VLIB_INVALID_HANDLE) {
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return;
	}

	hba_event = &event.hbaapi_event;
	memset(&hba_event->Event, 0, sizeof(hba_event->Event));

	hba_event->EventCode = fc_nle->event_code;
	switch (hba_event->EventCode) {
	case HBA_EVENT_LINK_UP:
	case HBA_EVENT_LINK_DOWN:
		hba_event->Event.Link_EventInfo.PortFcId = adapter->ident.did;
		break;
	case HBA_EVENT_RSCN:
		hba_event->Event.RSCN_EventInfo.PortFcId = adapter->ident.did;
		hba_event->Event.RSCN_EventInfo.NPortPage = fc_nle->event_data;
		break;
	default:
		VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
		return;
		break;
	}

	/* append that event to the linked list */
	appendEvent(&event, adapter);

	VLIB_MUTEX_UNLOCK(&vlib_data.mutex);
	return;
}

static void dispatch_event(struct nlmsghdr *nlh)
{
	struct scsi_nl_hdr *snlh = NULL;
	struct fc_nl_event *fc_nle = NULL;

	if (nlh->nlmsg_len > SCSITRANSPORT_MSG_SIZE)
		/* message not valid, discard */
		/* TODO: restliche bytes lesen */
		return;
	if (nlh->nlmsg_len < SCSITRANSPORT_MSG_SIZE)
		/* too short, discard as well */
		return;
	snlh = NLMSG_DATA(nlh);
	/* check if the message is a fc transport message */
	if (!snlh || snlh->transport != SCSI_NL_TRANSPORT_FC)
		return;
	fc_nle = NLMSG_DATA(nlh);
	if (!fc_nle)
		return;
	if (fc_nle->event_code == HBA_EVENT_LIP_OCCURRED ||
			fc_nle->event_code == HBA_EVENT_LIP_RESET_OCCURRED)
		/* should not occur, no FC-AL support on system z */
		return;
	process_event(fc_nle);
}

static void *establish_listener()
{
	struct msghdr msg;
	struct sockaddr_nl src_addr, dest_addr;
	struct nlmsghdr *nlh = NULL;
	struct scsi_nl_hdr *snlh = NULL;
	struct fc_nl_event *fc_nle = NULL;
	struct scsi_nl_host_vendor_msg *nlh_hvm = NULL;
	struct iovec iov;
	int sock_fd;
	int bytes_read;

	sock_fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_SCSITRANSPORT);
	memset(&src_addr, 0, sizeof(src_addr));
	src_addr.nl_family = AF_NETLINK;
	src_addr.nl_pid = getpid();
	/*src_addr.nl_groups = SCSI_NL_GRP_FC_EVENTS;*/
	src_addr.nl_groups = 8;

	bind(sock_fd, (struct sockaddr *)&src_addr, sizeof(src_addr));
	nlh = (struct nlmsghdr *)malloc(NLMSG_SPACE(SCSITRANSPORT_MSG_SIZE));
	memset(nlh, 0, NLMSG_SPACE(SCSITRANSPORT_MSG_SIZE));
	iov.iov_base = (void *)nlh;
	iov.iov_len = NLMSG_SPACE(SCSITRANSPORT_MSG_SIZE);
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	while (1) {
		/* Read message from kernel */
		bytes_read = recvmsg(sock_fd, &msg, 0);
		dispatch_event(nlh);
	}
}

void cleanup_event_thread()
{
}

void start_event_thread()
{
	pthread_create(&vlib_data.id, NULL, &establish_listener, NULL);
}
