/*
 * hdparm.c - Command line interface to get/set hard disk parameters.
 *          - by Mark Lord (C) 1994-2018 -- freely distributable.
 */
#define HDPARM_VERSION "v9.58"

#define _LARGEFILE64_SOURCE /*for lseek64*/
#define _BSD_SOURCE	/* for strtoll() */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#define __USE_GNU	/* for O_DIRECT */
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <endian.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <endian.h>
#include <asm/byteorder.h>

#include "hdparm.h"
#include "sgio.h"

static int    argc;
static char **argv;
static char  *argp;
static int    num_flags_processed = 0;

extern const char *minor_str[];

#ifndef O_DIRECT
#define O_DIRECT	040000	/* direct disk access, not easily obtained from headers */
#endif

#ifndef CDROM_SELECT_SPEED	/* already defined in 2.3.xx kernels and above */
#define CDROM_SELECT_SPEED	0x5322
#endif

#define TIMING_BUF_MB		2
#define TIMING_BUF_BYTES	(TIMING_BUF_MB * 1024 * 1024)

char *progname;
int verbose = 0;
int prefer_ata12 = 0;
static int do_defaults = 0, do_flush = 0, do_ctimings, do_timings = 0;
static int do_identity = 0, get_geom = 0, noisy = 1, quiet = 0;
static int do_flush_wcache = 0;

static int set_wdidle3  = 0, get_wdidle3 = 0, wdidle3 = 0;
static int   set_timings_offset = 0;
static __u64 timings_offset = 0;
static int set_fsreadahead= 0, get_fsreadahead= 0, fsreadahead= 0;
static int set_readonly = 0, get_readonly = 0, readonly = 0;
static int set_unmask   = 0, get_unmask   = 0, unmask   = 0;
static int set_mult     = 0, get_mult     = 0, mult     = 0;
static int set_dma      = 0, get_dma      = 0, dma      = 0;
static int set_dma_q	  = 0, get_dma_q    = 0, dma_q	  = 0;
static int set_nowerr   = 0, get_nowerr   = 0, nowerr   = 0;
static int set_keep     = 0, get_keep     = 0, keep     = 0;
static int set_io32bit  = 0, get_io32bit  = 0, io32bit  = 0;
static int set_piomode  = 0, get_piomode= 0, piomode = 0;
static int set_dkeep    = 0, get_dkeep    = 0, dkeep    = 0;
static int set_standby  = 0, get_standby  = 0, standby= 0;
			while (*argp) ++argp;
		}
		--num_flags_processed;	/* doesn't count as an action flag */
	} else if (0 == strcasecmp(name, "security-freeze")) {
		security_freeze = 1;
	} else {
		return handle_standalone_longarg(name);
	}
	return 0; /* additional flags allowed */
}
#endif

int main (int _argc, char **_argv)
{
	int no_more_flags = 0, disallow_flags = 0;
	char c;

	argc = _argc;
	argv = _argv;
	argp = NULL;

	if  ((progname = (char *) strrchr(*argv, '/')) == NULL)
		progname = *argv;
	else
		progname++;
	++argv;

	if (!--argc)
		usage_help(6,EINVAL);
	while (argc--) {
		argp = *argv++;
		if (no_more_flags || argp[0] != '-') {
			if (!num_flags_processed)
				do_defaults = 1;
			process_dev(argp);
			continue;
		}
		if (0 == strcmp(argp, "--")) {
			no_more_flags = 1;
			continue;
		}
		if (disallow_flags) {
			fprintf(stderr, "Excess flags given.\n");
			usage_help(7,EINVAL);
		}
		if (!*++argp)
			usage_help(8,EINVAL);
		while (argp && (c = *argp++)) {
			switch (c) {
				case GET_SET_PARM('a',"filesystem-read-ahead",fsreadahead,0,2048);
				case GET_SET_PARM('A',"look-ahead",lookahead,0,1);
				case GET_SET_PARM('b',"bus-state",busstate,0,2);
				case GET_SET_PARM('B',"power-management-mode",apmmode,0,255);
				case GET_SET_PARM('c',"32-bit-IO",io32bit,0,3);
				case     SET_FLAG('C',powermode);
				case GET_SET_PARM('d',"dma-enable",dma,0,1);
				case     SET_PARM('D',"defects-management",defects,0,1);
#if !defined (HDPARM_MINI)
				case     SET_PARM('E',"CDROM/DVD-speed",cdromspeed,0,255);
#endif
				case      DO_FLAG('f',do_flush);
				case      DO_FLAG('F',do_flush_wcache);
				case      DO_FLAG('g',get_geom);
				case              'h': usage_help(9,0); break;
				case     SET_FLAG('H',hitachi_temp);
				case      DO_FLAG('i',do_identity);
				case      DO_FLAG('I',do_IDentity);
				case GET_SET_PARM('J',"WDC-idle3-timeout",wdidle3,0,300);
				case GET_SET_PARM('k',"kernel-keep-settings",keep,0,1);
				case     SET_PARM('K',"drive-keep-settings",dkeep,0,1);
				case     SET_PARM('L',"door-lock",doorlock,0,1);
				case GET_SET_PARM('m',"multmode-count",mult,0,64);
				case GET_SET_PARM('M',"acoustic-management",acoustic,0,255);
				case GET_SET_PARM('n',"ignore-write-errors",nowerr,0,1);
#if !defined (HDPARM_MINI)
				case              'N': get_set_max_sectors_parms(); break;
#endif
				case     SET_PARM('P',"prefetch",prefetch,0,255);
				case              'q': quiet = 1; noisy = 0; break;
				case GET_SET_PARM('Q',"queue-depth",dma_q,0,1024);
				case     SET_PARM('s',"powerup-in-standby",powerup_in_standby,0,1);
				case     SET_PARM('S',"standby-interval",standby,0,255);
				case GET_SET_PARM('r',"read-only",readonly,0,1);
				case GET_SET_PARM('R',"write-read-verify",write_read_verify,0,3);
				case      DO_FLAG('t',do_timings);
				case      DO_FLAG('T',do_ctimings);
				case GET_SET_PARM('u',"unmask-irq",unmask,0,1);
				case      DO_FLAG('v',do_defaults);
				case              'V': fprintf(stdout, "%s %s\n", progname, HDPARM_VERSION); exit(0);
				case     SET_FLAG('w',doreset);
				case GET_SET_PARM('W',"write-cache",wcache,0,1);
				case     SET_FLAG('y',standbynow);
				case     SET_FLAG('Y',sleepnow);

				case     SET_FLAG('z',reread_partn);
				case     SET_FLAG('Z',seagate);

#if !defined (HDPARM_MINI)
				case '-':
					if (get_longarg())
						disallow_flags = 1;
					break;

				case 'p':
					get_piomode = noisy;
					noisy = 1;
					GET_XFERMODE(set_piomode,piomode);
					break;

				case 'X':
					get_xfermode = noisy;
					noisy = 1;
					GET_XFERMODE(set_xfermode,xfermode_requested);
					if (!set_xfermode)
						fprintf(stderr, "-X: missing value\n");
					break;
#endif

				default:
					usage_help(10,EINVAL);
			}
			num_flags_processed++;
		}
		if (!argc)
			usage_help(11,EINVAL);
	}
	return 0;
}
