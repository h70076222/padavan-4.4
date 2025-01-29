/* sgio.c - by Mark Lord (C) 2007 -- freely distributable */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <scsi/scsi.h>
#include <scsi/sg.h>

#include "sgio.h"
#include "hdparm.h"

#include <linux/hdreg.h>

extern int verbose;
extern int prefer_ata12;

static const unsigned int default_timeout_secs = 15;

/*
 * Taskfile layout for SG_ATA_16 cdb:
 *
 * LBA48:
 * cdb[ 3] = hob_feat
 * cdb[ 5] = hob_nsect
 * cdb[ 7] = hob_lbal
 * cdb[ 9] = hob_lbam
 * cdb[11] = hob_lbah
 *
 * LBA28/LBA48:
 * cdb[ 4] = feat
 * cdb[ 6] = nsect
 * cdb[ 8] = lbal
 * cdb[10] = lbam
 * cdb[12] = lbah
 * cdb[13] = device
 * cdb[14] = command
 *
 * Taskfile layout for SG_ATA_12 cdb:
 *
 * cdb[ 3] = feat
 * cdb[ 4] = nsect
 * cdb[ 5] = lbal
 * cdb[ 6] = lbam
 * cdb[ 7] = lbah
 * cdb[ 8] = device
 * cdb[ 9] = command
 *
 * dxfer_direction choices:
 *	SG_DXFER_TO_DEV, SG_DXFER_FROM_DEV, SG_DXFER_NONE
 */

#if 0  /* maybe use this in sg16 later.. ? */
static inline int get_rw (__u8 ata_op)
{
	switch (ata_op) {
		case ATA_OP_DSM:
		case ATA_OP_WRITE_PIO:
		case ATA_OP_WRITE_LONG:
		case ATA_OP_WRITE_LONG_ONCE:
		case ATA_OP_WRITE_PIO_EXT:
		case ATA_OP_WRITE_DMA_EXT:
		case ATA_OP_WRITE_FPDMA:
		case ATA_OP_WRITE_UNC_EXT:
		case ATA_OP_WRITE_DMA:
		case ATA_OP_SECURITY_UNLOCK:
		case ATA_OP_SECURITY_DISABLE:
		case ATA_OP_SECURITY_ERASE_UNIT:
		case ATA_OP_SECURITY_SET_PASS:
			return SG_WRITE;
	if (r->oflags.bits.lob.lbah)		tf.lob.lbah  = r->lob.lbah;
	if (r->oflags.bits.lob.dev)		tf.dev       = r->lob.dev;
	if (r->oflags.bits.lob.command)	tf.command   = r->lob.command;
	if (needs_lba48(tf.command,0,0) || r->oflags.bits.hob_all || r->iflags.bits.hob_all) {
		tf.is_lba48 = 1;
		if (r->oflags.bits.hob.feat)	tf.hob.feat  = r->hob.feat;
		if (r->oflags.bits.hob.lbal)	tf.hob.lbal  = r->hob.lbal;
		if (r->oflags.bits.hob.nsect)	tf.hob.nsect = r->hob.nsect;
		if (r->oflags.bits.hob.lbam)	tf.hob.lbam  = r->hob.lbam;
		if (r->oflags.bits.hob.lbah)	tf.hob.lbah  = r->hob.lbah;
		if (verbose)
			fprintf(stderr, "using LBA48 taskfile\n");
	}
	switch (r->cmd_req) {
		case TASKFILE_CMD_REQ_OUT:
		case TASKFILE_CMD_REQ_RAW_OUT:
			data_bytes = r->obytes;
			data       = r->data;
			rw         = SG_WRITE;
			break;
		case TASKFILE_CMD_REQ_IN:
			data_bytes = r->ibytes;
			data       = r->data;
			break;
	}

	rc = sg16(fd, rw, is_dma(tf.command), &tf, data, data_bytes, timeout_secs);
	if (rc == -1) {
		if (errno == EINVAL || errno == ENODEV || errno == EBADE)
			goto use_legacy_ioctl;
	}

	if (rc == 0 || errno == EIO) {
		if (r->iflags.bits.lob.feat)	r->lob.feat  = tf.error;
		if (r->iflags.bits.lob.lbal)	r->lob.lbal  = tf.lob.lbal;
		if (r->iflags.bits.lob.nsect)	r->lob.nsect = tf.lob.nsect;
		if (r->iflags.bits.lob.lbam)	r->lob.lbam  = tf.lob.lbam;
		if (r->iflags.bits.lob.lbah)	r->lob.lbah  = tf.lob.lbah;
		if (r->iflags.bits.lob.dev)	r->lob.dev   = tf.dev;
		if (r->iflags.bits.lob.command)	r->lob.command = tf.status;
		if (r->iflags.bits.hob.feat)	r->hob.feat  = tf.hob.feat;
		if (r->iflags.bits.hob.lbal)	r->hob.lbal  = tf.hob.lbal;
		if (r->iflags.bits.hob.nsect)	r->hob.nsect = tf.hob.nsect;
		if (r->iflags.bits.hob.lbam)	r->hob.lbam  = tf.hob.lbam;
		if (r->iflags.bits.hob.lbah)	r->hob.lbah  = tf.hob.lbah;
	}
	return rc;

use_legacy_ioctl:
#else
	timeout_secs = 0;	/* keep compiler happy */
#endif /* SG_IO */
	if (verbose)
		fprintf(stderr, "trying legacy HDIO_DRIVE_TASKFILE\n");
	errno = 0;

	rc = ioctl(fd, HDIO_DRIVE_TASKFILE, r);
	if (verbose) {
		int err = errno;
		fprintf(stderr, "rc=%d, errno=%d, returned ATA registers: ", rc, err);
		if (r->iflags.bits.lob.feat)	fprintf(stderr, " er=%02x", r->lob.feat);
		if (r->iflags.bits.lob.nsect)	fprintf(stderr, " ns=%02x", r->lob.nsect);
		if (r->iflags.bits.lob.lbal)	fprintf(stderr, " ll=%02x", r->lob.lbal);
		if (r->iflags.bits.lob.lbam)	fprintf(stderr, " lm=%02x", r->lob.lbam);
		if (r->iflags.bits.lob.lbah)	fprintf(stderr, " lh=%02x", r->lob.lbah);
		if (r->iflags.bits.lob.dev)	fprintf(stderr, " dh=%02x", r->lob.dev);
		if (r->iflags.bits.lob.command)	fprintf(stderr, " st=%02x", r->lob.command);
		if (r->iflags.bits.hob.feat)	fprintf(stderr, " err=%02x", r->hob.feat);
		if (r->iflags.bits.hob.nsect)	fprintf(stderr, " err=%02x", r->hob.nsect);
		if (r->iflags.bits.hob.lbal)	fprintf(stderr, " err=%02x", r->hob.lbal);
		if (r->iflags.bits.hob.lbam)	fprintf(stderr, " err=%02x", r->hob.lbam);
		if (r->iflags.bits.hob.lbah)	fprintf(stderr, " err=%02x", r->hob.lbah);
		fprintf(stderr, "\n");
		errno = err;
	}
	if (rc == -1 && errno == EINVAL) {
		fprintf(stderr, "The running kernel lacks CONFIG_IDE_TASK_IOCTL support for this device.\n");
		errno = EINVAL;
	}
	return rc;
}

void init_hdio_taskfile (struct hdio_taskfile *r, __u8 ata_op, int rw, int force_lba48,
				__u64 lba, unsigned int nsect, int data_bytes)
{
	memset(r, 0, sizeof(struct hdio_taskfile) + data_bytes);
	if (!data_bytes) {
		r->dphase  = TASKFILE_DPHASE_NONE;
		r->cmd_req = TASKFILE_CMD_REQ_NODATA;
	} else if (rw == RW_WRITE) {
		r->dphase  = TASKFILE_DPHASE_PIO_OUT;
		r->cmd_req = TASKFILE_CMD_REQ_RAW_OUT;
		r->obytes  = data_bytes;
	} else { /* rw == RW_READ */
		r->dphase  = TASKFILE_DPHASE_PIO_IN;
		r->cmd_req = TASKFILE_CMD_REQ_IN;
		r->ibytes  = data_bytes;
	}
	r->lob.command      = ata_op;
	r->oflags.bits.lob.command = 1;
	r->oflags.bits.lob.dev     = 1;
	r->oflags.bits.lob.lbal    = 1;
	r->oflags.bits.lob.lbam    = 1;
	r->oflags.bits.lob.lbah    = 1;
	r->oflags.bits.lob.nsect   = 1;

	r->iflags.bits.lob.command = 1;
	r->iflags.bits.lob.feat    = 1;

	r->lob.nsect = nsect;
	r->lob.lbal  = lba;
	r->lob.lbam  = lba >>  8;
	r->lob.lbah  = lba >> 16;
	r->lob.dev   = 0xa0 | ATA_USING_LBA;

	if (needs_lba48(ata_op, lba, nsect) || force_lba48) {
		r->hob.nsect = nsect >>  8;
		r->hob.lbal  = lba   >> 24;
		r->hob.lbam  = lba   >> 32;
		r->hob.lbah  = lba   >> 40;
		r->oflags.bits.hob.nsect = 1;
		r->oflags.bits.hob.lbal  = 1;
		r->oflags.bits.hob.lbam  = 1;
		r->oflags.bits.hob.lbah  = 1;
	} else {
		r->lob.dev |= (lba >> 24) & 0x0f;
	}
}
