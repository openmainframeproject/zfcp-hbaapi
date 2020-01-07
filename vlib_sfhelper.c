/*
 * Copyright IBM Corp. 2003,2010
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Common Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.ibm.com/developerworks/library/os-cpl.html
 *
 * Authors:		Sven Schuetz <sven@de.ibm.com>
 *
 * File:		vlib_sfhelper.c
 *
 * Description:
 * sysfs helper functions to hide the resource access
 *
 */


#include "vlib.h"


sfhelper_dir *sfhelper_opendir(char *dirname)
{
	return (sfhelper_dir *)(opendir(dirname));
}

void sfhelper_closedir(sfhelper_dir *dir)
{
	closedir((DIR *)dir);
}

char *sfhelper_getNextDirEnt(sfhelper_dir *dir)
{
	struct dirent *direntry;

	while (direntry = readdir(dir))
		return direntry->d_name;
	return NULL;
}

int sfhelper_getProperty(char *dir, char *name, char *result)
{
	FILE *fd;
	char path[PATH_MAX];

	snprintf(path, PATH_MAX, "%s/%s", dir, name);

	fd = fopen(path, "r");
	if (!fd)
		return 0;
	if (fgets(result, ATTR_MAX, fd) == NULL)
		return -1;
	fclose(fd);
	if (result[strlen(result) - 1] == '\n')
		result[strlen(result) - 1] = '\0';
	return 0;
}
