#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>
#include <scsi/scsi.h>
#include <scsi/sg.h>
#include <sys/types.h>
#include <errno.h>
#include "hdparm.h"
#include "sgio.h"

/*
 * apt - Support for ATA PASS THROUGH devices. Currently supported only
 *       JMicron devices.
 *
 * Copyright (c) 2009	Jan Friesse <jfriesse@gmail.com>
 *
 * Magic numbers are taken from smartmontools source code
 * (http://smartmontools.sourceforge.net/)
 *
 * You may use/distribute this freely, under the terms of either
 * (your choice) the GNU General Public License version 2,
 * or a BSD style license.
 */

#ifdef SG_IO

/* Device initialization functions */
static int apt_jmicron_int_init(int fd);

/* Device sg16 functions*/
static int apt_jmicron_sg16(int fd, int rw, int dma, struct ata_tf *tf,
	    void *data, unsigned int data_bytes, unsigned int timeout_secs);

/* Structs */
struct apt_usb_id_entry {
	int vendor_id;
	int product_id;
	int version;
	const char *type;
	int (*init_func)(int fd);
	int (*sg16_func)(int fd, int rw, int dma, struct ata_tf *tf,
	    void *data, unsigned int data_bytes, unsigned int timeout_secs);

};

struct apt_data_struct {
	int is_apt;
	struct apt_usb_id_entry id;
	int verbose;
	union {
		struct {
			int port;
		} jmicron;
	};
};

static struct apt_data_struct apt_data;

const char apt_ds_jmicron[] = "jmicron";
const char apt_ds_unsup[]   = "unsupported";

const struct apt_usb_id_entry apt_usb_id_map[] = {
	{0x152d, 0x2329, 0x0100, apt_ds_jmicron,
	    apt_jmicron_int_init, apt_jmicron_sg16}, /* JMicron JM20329 (USB->SATA) */
	{0x152d, 0x2336, 0x0100, apt_ds_jmicron,
	    apt_jmicron_int_init, apt_jmicron_sg16}, /* JMicron JM20336 (USB+SATA->SATA, USB->2xSATA) */
	{0x152d, 0x2338, 0x0100, apt_ds_jmicron,
	    apt_jmicron_int_init, apt_jmicron_sg16}, /* JMicron JM20337/8 (USB->SATA+PATA, USB+SATA->PATA) */
	{0x152d, 0x2339, 0x0100, apt_ds_jmicron,
	    apt_jmicron_int_init, apt_jmicron_sg16}  /* JMicron JM20339 (USB->SATA) */
};

int apt_detect (int fd, int verbose)
{
	int err;
	unsigned int i;

	apt_data.is_apt = 0;

	err = sysfs_get_attr_recursive(fd, "idVendor", "%x", &apt_data.id.vendor_id, NULL, verbose);
	if (err) {
		if (verbose) printf("APT: No idVendor found -> not USB bridge device\n");
		return 0;
	}

	err = sysfs_get_attr_recursive(fd, "idProduct", "%x", &apt_data.id.product_id, NULL, verbose);
	if (err) return 0;

	err = sysfs_get_attr_recursive(fd, "bcdDevice", "%x", &apt_data.id.version, NULL, verbose);
	if (err) return 0;

	if (verbose)
		printf("APT: USB ID = 0x%04x:0x%04x (0x%03x)\n", apt_data.id.vendor_id, apt_data.id.product_id,
		  apt_data.id.version);

	/* We have all needed informations, let's find if we support that device*/
	for (i = 0; i <	sizeof(apt_usb_id_map)/sizeof(*apt_usb_id_map); i++) {
		if (apt_data.id.vendor_id == apt_usb_id_map[i].vendor_id &&
		    apt_data.id.product_id == apt_usb_id_map[i].product_id) {
			/* Maybe two devices with same vendor and product id -> use version*/
			if (apt_usb_id_map[i].version > 0 && apt_data.id.type &&
			    apt_usb_id_map[i].version == apt_data.id.version) {
				apt_data.id.type = apt_usb_id_map[i].type;
				apt_data.id.init_func = apt_usb_id_map[i].init_func;
				apt_data.id.sg16_func = apt_usb_id_map[i].sg16_func;
			}

			/* We don't have type -> set it (don't care about version) */
			if (!apt_data.id.type) {
				apt_data.id.type = apt_usb_id_map[i].type;
				apt_data.id.init_func = apt_usb_id_map[i].init_func;
				apt_data.id.sg16_func = apt_usb_id_map[i].sg16_func;
			}

	return 0;
}

static int apt_jmicron_int_get_registers(int fd, unsigned short addr,
				    unsigned char * buf, unsigned short size)
{
	struct ata_tf tf;

	memset(&tf, 0, sizeof(tf));

	tf.lob.feat	= 0x00;
	tf.lob.nsect	= (unsigned char)(addr >> 8);
	tf.lob.lbal	= (unsigned char)(addr);
	tf.lob.lbam	= 0x00;
	tf.lob.lbah	= 0x00;
	tf.command	= 0xfd;

	return apt_jmicron_int_sg(fd, 0, 0, &tf, buf, (unsigned int)size, 0, 0x00);
}

static int apt_jmicron_int_init(int fd)
{
	unsigned char regbuf = 0;
	int res;

	if ((res = apt_jmicron_int_get_registers(fd, 0x720F, &regbuf, 1)) == -1) {
		return res;
	}

	if (regbuf & 0x04) {
		apt_data.jmicron.port = 0xa0;
	} else if (regbuf & 0x40) {
		apt_data.jmicron.port = 0xb0;
	} else {
		perror("APT: No JMicron device connected");
		errno = ENODEV;
		return -1;
	}

	if (apt_data.verbose)
		printf("APT: JMicron Port: 0x%X\n", apt_data.jmicron.port);
	return 0;
}

static int apt_jmicron_sg16(int fd, int rw, int dma, struct ata_tf *tf,
        void *data, unsigned int data_bytes, unsigned int timeout_secs)
{

	return apt_jmicron_int_sg(fd, rw, dma, tf, data, data_bytes, timeout_secs, 0);
}

#else
/* No SGIO -> no support*/
int apt_detect (int fd, int verbose)
{
	if (verbose)
		printf("APT: SGIO Support needed for fd %d\n", fd);
	return 0;
}

int apt_is_apt (void)
{
	return 0;
}

int apt_sg16(int fd, int rw, int dma, struct ata_tf *tf,
	    void *data, unsigned int data_bytes, unsigned int timeout_secs)
{
	printf("APT: SG16 fd %d rw %d dma %d tf %p data %p data_bytes %d timeout %d need SGIO\n",
		fd, rw, dma, tf, data, data_bytes, timeout_secs);
	return -1;
}
#endif
