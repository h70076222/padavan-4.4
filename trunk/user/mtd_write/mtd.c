/*
 * mtd - simple memory technology device manipulation tool
 *
 * Copyright (C) 2005 Waldemar Brodkorb <wbx@dass-it.de>,
 *	                  Felix Fietkau <nbd@openwrt.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id: mtd.c,v 1.1 2009-03-17 09:48:42 steven Exp $
 *
 * The code is based on the linux-mtd examples.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <linux/reboot.h>

#include <mtd-abi.h>

#define BUFSIZE		(1024)
#define MAX_ARGS	3

#ifndef ROUNDUP
#define ROUNDUP(x, y)	((((x)+((y)-1))/(y))*(y))
#endif

static int quiet = 0;
static int no_erase = 0;
static int write_check = 1;

static int
mtd_open(const char *mtd, int flags, struct mtd_info_user *p_mi)
{
	FILE *fp;
	char line[128], bnm[64], *p;
	int i, idx, fd;

	idx = -1;

	if ((fp = fopen("/proc/mtd", "r"))) {
		fgets(line, sizeof(line), fp); //skip the 1st line
		while (fgets(line, sizeof(line), fp)) {
			if (sscanf(line, "mtd%d: %*s %*s \"%s\"", &i, bnm) > 1) {
				/* strip tailed " character, if present. */
				if ((p = strchr(bnm, '"')) != NULL)
					*p = '\0';
				if (!strcmp(bnm, mtd)) {
					idx = i;
					break;
				}
			}
		}
		fclose(fp);
	image_file = "<stdin>";

	part_ofs = 0;

	while ((ch = getopt(argc, argv, "wrnqe:o:l:p:")) != -1) {
		switch (ch) {
			case 'w':
				write_check = 0;
				break;
			case 'r':
				do_reboot = 1;
				break;
			case 'n':
				no_erase = 1;
				break;
			case 'q':
				quiet++;
				break;
			case 'e':
				i = 0;
				while ((erase[i] != NULL) && ((i + 1) < MAX_ARGS))
					i++;
					
				erase[i++] = optarg;
				erase[i] = NULL;
				break;
			case 'o':
				image_ofs = strtoll(optarg, NULL, 0);
				break;
			case 'l':
				image_len = strtoll(optarg, NULL, 0);
				break;
			case 'p':
				part_ofs = strtoul(optarg, 0, 0);
				break;
			case '?':
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 2)
		usage();

	if ((strcmp(argv[0], "unlock") == 0) && (argc == 2)) {
		cmd = CMD_UNLOCK;
		device = argv[1];
	} else if ((strcmp(argv[0], "erase") == 0) && (argc == 2)) {
		cmd = CMD_ERASE;
		device = argv[1];
	} else if ((strcmp(argv[0], "write") == 0) && (argc == 3)) {
		cmd = CMD_WRITE;
		device = argv[2];
		
		if (strcmp(argv[1], "-") != 0) {
			image_file = argv[1];
			if ((image_fd = open(image_file, O_RDONLY)) < 0) {
				fprintf(stderr, "Couldn't open image file: '%s'!\n", image_file);
				exit(1);
			}
			
			if (image_len == 0) {
				struct stat st;
				if (fstat(image_fd, &st) < 0) {
					fprintf(stderr, "Couldn't get stat of image file '%s' (errno: %d)!\n", image_file, errno);
					close(image_fd);
					exit(1);
				}
				if (st.st_size < 1) {
					fprintf(stderr, "Image file '%s' is empty!\n", image_file);
					close(image_fd);
					exit(1);
				}
				
				image_len = st.st_size;
				if (image_ofs > 0 && image_len > image_ofs) {
					if (lseek(image_fd, image_ofs, SEEK_SET) < 0) {
						fprintf(stderr, "Unable seek to offset 0x%lx in file '%s' (errno: %d)!\n", image_ofs, image_file, errno);
						close(image_fd);
						exit(1);
					}
					image_len -= image_ofs;
				}
			}
		}
	} else {
		usage();
	}

	sync();

	i = 0;
	unlocked = 0;
	while (erase[i] != NULL) {
		mtd_unlock(erase[i]);
		mtd_erase(erase[i]);
		if (strcmp(erase[i], device) == 0)
			unlocked = 1;
		i++;
	}

	if (!unlocked)
		mtd_unlock(device);

	switch (cmd) {
		case CMD_UNLOCK:
			break;
		case CMD_ERASE:
			mtd_erase(device);
			break;
		case CMD_WRITE:
			mtd_write(device, part_ofs, image_file, image_fd, image_len);
			break;
	}

	sync();

	if (do_reboot) {
		fprintf(stderr, "Rebooting ...\n");
		fflush(stderr);
		syscall(SYS_reboot,LINUX_REBOOT_MAGIC1,LINUX_REBOOT_MAGIC2,LINUX_REBOOT_CMD_RESTART,NULL);
	}

	return 0;
}
