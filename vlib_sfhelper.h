/*
 * Copyright IBM Corp. 2003,2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:		Sven Schuetz <sven@de.ibm.com>
 *
 * File:		vlib_sfhelper.h
 *
 * Description:
 * sysfs helper functions to hide the resource access
 *
 */

#ifndef VLIB_SFHELPER_H_
#define VLIB_SFHELPER_H_


typedef DIR sfhelper_dir;

sfhelper_dir *sfhelper_opendir(char *);
void sfhelper_closedir(sfhelper_dir *);
char *sfhelper_getNextDirEnt(sfhelper_dir *);
int sfhelper_getProperty(char *, char *, char *);

#endif /*VLIB_SFHELPER_H_*/
