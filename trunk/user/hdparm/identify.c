/* identify.c - by Mark Lord (C) 2000-2007 -- freely distributable */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <linux/types.h>
#include <endian.h>

#if __BYTE_ORDER == __BIG_ENDIAN
#define __USE_XOPEN
#endif

#include "hdparm.h"

/* device types */
/* ------------ */
#define NO_DEV                  0xffff
#define ATA_DEV                 0x0000
#define ATAPI_DEV               0x0001

/* word definitions */
/* ---------------- */
#define GEN_CONFIG		0   /* general configuration */
#define LCYLS			1   /* number of logical cylinders */
#define CONFIG			2   /* specific configuration */
#define LHEADS			3   /* number of logical heads */
#define TRACK_BYTES		4   /* number of bytes/track (ATA-1) */
#define SECT_BYTES		5   /* number of bytes/sector (ATA-1) */
#define LSECTS			6   /* number of logical sectors/track */
#define START_SERIAL            10  /* ASCII serial number */
#define LENGTH_SERIAL           10  /* 10 words (20 bytes or characters) */
#define BUF_TYPE		20  /* buffer type (ATA-1) */
#define BUF_SIZE		21  /* buffer size (ATA-1) */
#define RW_LONG			22  /* extra bytes in R/W LONG cmd ( < ATA-4)*/
#define START_FW_REV            23  /* ASCII firmware revision */
#define LENGTH_FW_REV		 4  /*  4 words (8 bytes or characters) */
#define START_MODEL    		27  /* ASCII model number */
#define LENGTH_MODEL    	20  /* 20 words (40 bytes or characters) */
#define SECTOR_XFER_MAX		47  /* r/w multiple: max sectors xfered */
#define DWORD_IO		48  /* can do double-word IO (ATA-1 only) */
#define CAPAB_0			49  /* capabilities */
#define CAPAB_1			50
#define PIO_MODE		51  /* max PIO mode supported (obsolete)*/
#define DMA_MODE		52  /* max Singleword DMA mode supported (obs)*/
#define WHATS_VALID		53  /* what fields are valid */
#define LCYLS_CUR		54  /* current logical cylinders */
#define LHEADS_CUR		55  /* current logical heads */
#define LSECTS_CUR		56  /* current logical sectors/track */
#define CAPACITY_LSB		57  /* current capacity in sectors */
#define CAPACITY_MSB		58
#define SECTOR_XFER_CUR		59  /* r/w multiple: current sectors xfered */
#define LBA_SECTS_LSB		60  /* LBA: total number of user */
#define LBA_SECTS_MSB		61  /*      addressable sectors */
#define SINGLE_DMA		62  /* singleword DMA modes */
#define MULTI_DMA		63  /* multiword DMA modes */
#define ADV_PIO_MODES		64  /* advanced PIO modes supported */
				    /* multiword DMA xfer cycle time: */
#define DMA_TIME_MIN		65  /*   - minimum */
#define DMA_TIME_NORM		66  /*   - manufacturer's recommended	*/
				    /* minimum PIO xfer cycle time: */
#define PIO_NO_FLOW		67  /*   - without flow control */
#define PIO_FLOW		68  /*   - with IORDY flow control */
#define PKT_REL			71  /* typical #ns from PKT cmd to bus rel */
#define SVC_NBSY		72  /* typical #ns from SERVICE cmd to !BSY */
#define CDR_MAJOR		73  /* CD ROM: major version number */
#define CDR_MINOR		74  /* CD ROM: minor version number */
#define QUEUE_DEPTH		75  /* queue depth */
#define SATA_CAP_0		76  /* Serial ATA Capabilities */
#define SATA_RESERVED_77	77  /* reserved for future Serial ATA definition */
#define SATA_SUPP_0		78  /* Serial ATA features supported */
#define SATA_EN_0		79  /* Serial ATA features enabled */
#define MAJOR			80  /* major version number */
#define MINOR			81  /* minor version number */
#define CMDS_SUPP_0		82  /* command/feature set(s) supported */
		if(val[HWRST_RSLT] & CBLID) printf("\tCBLID- above Vih\n");
		else			    printf("\tCBLID- below Vih\n");
		if(val[HWRST_RSLT] & RST0)  {
			printf("\tDevice num = 0");
			jj = val[HWRST_RSLT];
		} else {
			printf("\tDevice num = 1");
			jj = val[HWRST_RSLT] >> 8;
		}
		if((jj & DEV_DET) == JUMPER_VAL) 
			printf(" determined by the jumper");
		else if((jj & DEV_DET) == CSEL_VAL)
			printf(" determined by CSEL");
		printf("\n");
	}
	print_devslp_info(fd, val);

	/* more stuff from std 5 */
	if ((like_std > 4) && (eqpt != CDROM)) {
		if ((val[INTEGRITY] & SIG) == SIG_VAL) {
			printf("Checksum: %scorrect", chksum ? "in" : "");
			if (chksum)
				printf(" (0x%02x), expected 0x%02x\n", chksum, 0x100 - chksum);
			putchar('\n');
		} else {
			printf("Integrity word not set (found 0x%04x, expected 0x%02x%02x)\n",
				val[INTEGRITY], 0x100 - chksum, SIG_VAL);
		}
	}
}

__u8 mode_loop(__u16 mode_sup, __u16 mode_sel, int cc, __u8 *have_mode) {
	__u16 ii;
	__u8 err_dma = 0;
	for (ii = 0; ii <= MODE_MAX; ii++) {
		if(mode_sel & 0x0001) {
			printf("*%cdma%u ",cc,ii);
			if(*have_mode) err_dma = 1;
			*have_mode = 1;
		} else if(mode_sup & 0x0001) {
			printf("%cdma%u ",cc,ii);
		}
		mode_sup >>=1;   mode_sel >>=1;
	}
	return err_dma;
}

void dco_identify_print (__u16 *dco)
{
	__u64 lba;

	printf("DCO Revision: 0x%04x", dco[0]);
	if (dco[0] == 0 || dco[0] > 2)
		printf(" -- unknown, treating as 0002");
	printf("\nThe following features can be selectively disabled via DCO:\n");

	printf("\tTransfer modes:\n\t\t");
	if (dco[1] & 0x0007) {
		     if (dco[1] & (1<<2)) printf(" mdma0 mdma1 mdma2");
		else if (dco[1] & (1<<1)) printf(" mdma0 mdma1");
		else if (dco[1] & (1<<0)) printf(" mdma0");
		printf("\n\t\t");
	}
	if (dco[2] & (1<<6)) {
		printf(" udma0 udma1 udma2 udma3 udma4 udma5 udma6");
		if (dco[0] < 2)
			printf("(?)");
	}
	else if (dco[2] & (1<<5)) printf(" udma0 udma1 udma2 udma3 udma4 udma5");
	else if (dco[2] & (1<<4)) printf(" udma0 udma1 udma2 udma3 udma4");
	else if (dco[2] & (1<<3)) printf(" udma0 udma1 udma2 udma3");
	else if (dco[2] & (1<<2)) printf(" udma0 udma1 udma2");
	else if (dco[2] & (1<<1)) printf(" udma0 udma1");
	else if (dco[2] & (1<<0)) printf(" udma0");
	putchar('\n');

	lba = ((((__u64)dco[5]) << 32) | (dco[4] << 16) | dco[3]) + 1;
	printf("\tReal max sectors: %llu\n", lba);

	printf("\tATA command/feature sets:");
	if (dco[7] & 0x01ff) {
		printf("\n\t\t");
		if (dco[7] & (1<< 0)) printf(" SMART");
		if (dco[7] & (1<< 1)) printf(" self_test");
		if (dco[7] & (1<< 2)) printf(" error_log");
		if (dco[7] & (1<< 3)) printf(" security");
		if (dco[7] & (1<< 4)) printf(" PUIS");
		if (dco[7] & (1<< 5)) printf(" TCQ");
		if (dco[7] & (1<< 6)) printf(" AAM");
		if (dco[7] & (1<< 7)) printf(" HPA");
		if (dco[7] & (1<< 8)) printf(" 48_bit");
	}
	if (dco[7] & 0xfe00) {
		printf("\n\t\t");
		if (dco[0] < 2)
			printf(" (?):");
		if (dco[7] & (1<< 9)) printf(" streaming");
		if (dco[7] & (1<<10)) printf(" TLC_Reserved_7[10]");
		if (dco[7] & (1<<11)) printf(" FUA");
		if (dco[7] & (1<<12)) printf(" selective_test");
		if (dco[7] & (1<<13)) printf(" conveyance_test");
		if (dco[7] & (1<<14)) printf(" write_read_verify");
		if (dco[7] & (1<<15)) printf(" reserved_7[15]");
	}
	if (dco[21] & 0xf800) {
		printf("\n\t\t");
		if (dco[0] < 2)
			printf(" (?):");
		if (dco[21] & (1<<11)) printf(" free_fall");
		if (dco[21] & (1<<12)) printf(" trusted_computing");
		if (dco[21] & (1<<13)) printf(" WRITE_UNC_EXT");
		if (dco[21] & (1<<14)) printf(" NV_cache_power_management");
		if (dco[21] & (1<<15)) printf(" NV_cache");
	}
	putchar('\n');

	if (dco[8] & 0x1f) {
		printf("\tSATA command/feature sets:\n\t\t");
		if (dco[0] < 2)
			printf(" (?):");
		if (dco[8] & (1<<0)) printf(" NCQ");
		if (dco[8] & (1<<1)) printf(" NZ_buffer_offsets");
		if (dco[8] & (1<<2)) printf(" interface_power_management");
		if (dco[8] & (1<<3)) printf(" async_notification");
		if (dco[8] & (1<<4)) printf(" SSP");
		putchar('\n');
	}
}
