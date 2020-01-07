/*
 * Copyright IBM Corp. 2003,2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:		Sven Schuetz <sven@de.ibm.com>
 *
 * File:		vlib_events.h
 *
 * Description:
 * Event handling functions
 *
 */


#ifndef VLIB_EVENTS_H_
#define VLIB_EVENTS_H_

void free_event_queue(struct vlib_adapter *);
void init_event_queue(struct vlib_adapter *);
void start_event_thread();
struct vlib_event *popEvent(struct vlib_adapter *);

#endif /*VLIB_EVENTS_H_*/
