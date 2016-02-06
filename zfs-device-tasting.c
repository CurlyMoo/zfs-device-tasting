/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2015 CurlyMo, Inc. All rights reserved.
 */
 
 /*
  * Compile using gcc -g -Wall -o zfs-device-tasting zfs-device-tasting.c
  */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>

#define DEFAULT_IMPORT_PATH_SIZE	8
#define BUFFER_SIZE								16

/*
 * Disk link priority for importing
 */
struct diskstypes_t {
	char *name;
	char *path;
} diskstypes[DEFAULT_IMPORT_PATH_SIZE] = {
	{ "by-vdev", "/dev/disk/by-vdev" },
	{ "mapper", "/dev/mapper" },
	{ "by-partlabel", "/dev/disk/by-partlabel" },
	{ "by-partuuid", "/dev/disk/by-partuuid" },
	{ "by-label", "/dev/disk/by-label" },
	{ "by-uuid", "/dev/disk/by-uuid" },
	{ "by-id", "/dev/disk/by-id" },
	{ "by-path", "/dev/disk/by-path" }
};

/*
 * Block devices with their respective symlinks
 * in priority order.
 */
struct symlinks_t {
	char dev[PATH_MAX];
	char link[PATH_MAX];
} symlinks_t;

struct symlinks_t *symlinks = NULL;

int main(void) {
	
	struct dirent *file = NULL;
	DIR *d;
	struct stat s;
	int nrblk = 0, size = 0, i = 0, x = 0;
	ssize_t bytes = 0;
	char path[2][PATH_MAX], link[PATH_MAX], *abs = NULL;

	/*
	 * First get all blockdevices from /dev
	 */
	if((d = opendir("/dev"))) {
		while((file = readdir(d)) != NULL) {
			memset(path[0], '\0', PATH_MAX);
			snprintf(path[0], PATH_MAX, "/dev/%s", file->d_name);
			if(stat(path[0], &s) == 0) {
				/* Check if blockdevice */
				if(S_ISBLK(s.st_mode)) {
					if(nrblk >= size) {
						if((symlinks = realloc(symlinks, sizeof(struct symlinks_t)*(size+BUFFER_SIZE))) == NULL) {
							fprintf(stderr, "out of memory");
							exit(-1);
						}
						memset(&symlinks[size], '\0', sizeof(struct symlinks_t)*BUFFER_SIZE);
						size += BUFFER_SIZE;
					}
					memset(symlinks[nrblk].dev, '\0', strlen(file->d_name)+1);
					strcpy(symlinks[nrblk].dev, file->d_name);
					memset(&symlinks[nrblk].link, '\0', PATH_MAX);
					nrblk++;
				}
			}
		}
		closedir(d);
	}
	for(i=0;i<DEFAULT_IMPORT_PATH_SIZE;i++) {
		/*
		 * Check all disk link paths for symlinks
		 * matching blockdevices, and store the
		 * one with the highest priority.
		 */
		if((d = opendir(diskstypes[i].path))) {
			while((file = readdir(d)) != NULL) {								
				memset(path[0], '\0', PATH_MAX);
				snprintf(path[0], PATH_MAX, "%s/%s", diskstypes[i].path, file->d_name);
				if(stat(path[0], &s) == 0) {
					if((bytes = readlink(path[0], link, PATH_MAX)) > 0) {
						link[bytes] = '\0';
						memset(path[1], '\0', PATH_MAX);
						snprintf(path[1], PATH_MAX, "%s/%s", diskstypes[i].path, link);
						if((abs = realpath(path[1], NULL)) != NULL) {
							for(x=0;x<nrblk;x++) {
								if(strcmp(basename(abs), symlinks[x].dev) == 0 && 
								   strlen(symlinks[x].link) == 0) {
									strcpy(symlinks[x].link, path[0]);
								}
							}
							free(abs);
						}
					}
				}
			}
			closedir(d);
		}
	}
	
	for(i=0;i<nrblk;i++) {
		printf("%s\n", symlinks[i].dev);
		if(strlen(symlinks[i].link) > 0) {
			printf("- %s\n", symlinks[i].link);
		}
	}
	free(symlinks);
	return 0;
}
