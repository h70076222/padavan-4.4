/*
 * mtd - simple memory technology device manipulation tool
 *
 * Copyright (C) 2005      Waldemar Brodkorb <wbx@dass-it.de>,
 * Copyright (C) 2005-2009 Felix Fietkau <nbd@nbd.name>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License v2
 * as published by the Free Software Foundation.
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
 *
 * The code is based on the linux-mtd examples.
 */

#define _GNU_SOURCE
#include <byteswap.h>
#include <endian.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <linux/reboot.h>
#include <mtd/mtd-user.h>
//#include "fis.h"
#include "mtd.h"

//#include <libubox/md5.h>

#define MAX_ARGS 8
#define JFFS2_DEFAULT_DIR	"" /* directory name without /, empty means root dir */

#define TRX_MAGIC		0x48445230	/* "HDR0" */
#define SEAMA_MAGIC		0x5ea3a417
#define WRGG03_MAGIC		0x20080321

#if !defined(__BYTE_ORDER)
#error "Unknown byte order"
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#define cpu_to_be32(x)	(x)
#define be32_to_cpu(x)	(x)
#define le32_to_cpu(x)	bswap_32(x)
#elif __BYTE_ORDER == __LITTLE_ENDIAN
#define cpu_to_be32(x)	bswap_32(x)
#define be32_to_cpu(x)	bswap_32(x)
#define le32_to_cpu(x)  (x)
#else
#error "Unsupported endianness"
#endif

enum mtd_image_format {

	if ((strcmp(argv[0], "unlock") == 0) && (argc == 2)) {
		cmd = CMD_UNLOCK;
		device = argv[1];
	} else if ((strcmp(argv[0], "erase") == 0) && (argc == 2)) {
		cmd = CMD_ERASE;
		device = argv[1];
	} else if (((strcmp(argv[0], "resetbc") == 0) && (argc == 2)) && mtd_resetbc) {
		cmd = CMD_RESETBC;
		device = argv[1];
	} else if (((strcmp(argv[0], "fixtrx") == 0) && (argc == 2)) && mtd_fixtrx) {
		cmd = CMD_FIXTRX;
		device = argv[1];
	} else if (((strcmp(argv[0], "fixseama") == 0) && (argc == 2)) && mtd_fixseama) {
		cmd = CMD_FIXSEAMA;
		device = argv[1];
	} else if (((strcmp(argv[0], "fixwrgg") == 0) && (argc == 2)) && mtd_fixwrgg) {
		cmd = CMD_FIXWRGG;
		device = argv[1];
	} else if ((strcmp(argv[0], "verify") == 0) && (argc == 3)) {
		cmd = CMD_VERIFY;
		imagefile = argv[1];
		device = argv[2];
	} else if ((strcmp(argv[0], "dump") == 0) && (argc == 2)) {
		cmd = CMD_DUMP;
		device = argv[1];
	} else if ((strcmp(argv[0], "write") == 0) && (argc == 3)) {
		cmd = CMD_WRITE;
		device = argv[2];

		if (strcmp(argv[1], "-") == 0) {
			imagefile = "<stdin>";
			imagefd = 0;
		} else {
			imagefile = argv[1];
			if ((imagefd = open(argv[1], O_RDONLY)) < 0) {
				fprintf(stderr, "Couldn't open image file: %s!\n", imagefile);
				exit(1);
			}
		}

		if (!mtd_check(device)) {
			fprintf(stderr, "Can't open device for writing!\n");
			exit(1);
		}
		/* check trx file before erasing or writing anything */
		if (!image_check(imagefd, device) && !force) {
			fprintf(stderr, "Image check failed.\n");
			exit(1);
		}
	} else if ((strcmp(argv[0], "jffs2write") == 0) && (argc == 3)) {
		cmd = CMD_JFFS2WRITE;
		device = argv[2];

		imagefile = argv[1];
		if (!mtd_check(device)) {
			fprintf(stderr, "Can't open device for writing!\n");
			exit(1);
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

	switch (cmd) {
		case CMD_UNLOCK:
			if (!unlocked)
				mtd_unlock(device);
			break;
		case CMD_VERIFY:
		//	mtd_verify(device, imagefile);
			break;
		case CMD_DUMP:
			mtd_dump(device, offset, dump_len);
			break;
		case CMD_ERASE:
			if (!unlocked)
				mtd_unlock(device);
			mtd_erase(device);
			break;
		case CMD_WRITE:
			if (!unlocked)
				mtd_unlock(device);
			mtd_write(imagefd, device, fis_layout, part_offset);
			break;
		case CMD_JFFS2WRITE:
			if (!unlocked)
				mtd_unlock(device);
			mtd_write_jffs2(device, imagefile, jffs2dir);
			break;
		case CMD_FIXTRX:
			if (mtd_fixtrx) {
				mtd_fixtrx(device, offset, data_size);
			}
			break;
		case CMD_RESETBC:
			if (mtd_resetbc) {
				mtd_resetbc(device);
			}
			break;
		case CMD_FIXSEAMA:
			if (mtd_fixseama)
				mtd_fixseama(device, 0, data_size);
			break;
		case CMD_FIXWRGG:
			if (mtd_fixwrgg)
				mtd_fixwrgg(device, 0, data_size);
			break;
	}

	sync();

	if (boot)
		do_reboot();

	return 0;
}
