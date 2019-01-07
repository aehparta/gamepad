/*
 * Gamepad daemon device
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <errno.h>
#include <libe/debug.h>
#include <libe/os.h>
#include "cmd.h"


#ifdef USE_FTDI
/* usb vid */
uint16_t usb_vid = 0x0403;
/* usb pid */
uint16_t usb_pid = 0x6014;
/* usb description */
const char *usb_description = "nRF24L01+";
/* usb serial */
const char *usb_serial = NULL;
/* interface (defaults to first one) */
int interface = INTERFACE_ANY;
/* reset flag, reset usb device if this is set */
int reset = 0;
#endif


void common_help(int argc, char *argv[])
{
	printf(
	    "\n"
	    "Usage:\n"
	    " %s [options]\n"
#ifdef USE_FTDI
	    "\n"
	    "Definitions for options:\n"
	    " ID = hexadecimal word\n"
	    " INTERFACE = integer between 1 and 4 depending on device type\n"
#endif
	    "\n"
	    "Options:\n"
#ifdef USE_FTDI
	    "  -h, --help                 display this help and exit\n"
	    "  -V, --vid=ID               usb vendor id, default 0403\n"
	    "  -P, --pid=ID               usb product id, default 6014\n"
	    "                             as default vid and pid are zero, so any first compatible ftdi device is used\n"
	    "  -D, --description=STRING   usb description (product) to use for opening right device, default \"nRF24L01+\"\n"
	    "  -S, --serial=STRING        usb serial to use for opening right device, default none\n"
	    "  -I, --interface=INTERFACE  ftx232 interface number, defaults to any available, set to 5 to disable FTDI totally\n"
	    "  -R, --reset                do usb reset on the device at start\n"
	    "  -L, --list-devices         list all devices found\n"
#endif
	    "\n"
	    , basename(argv[0]));
	p_help();
}

int common_options(int argc, char *argv[], const char opts[], struct option longopts[])
{
	int err = 0;
	int longindex = 0, c;
#ifdef USE_FTDI
	int i;
#endif

	while ((c = getopt_long(argc, argv, opts, longopts, &longindex)) > -1) {
		/* check for command specific options */
		err = p_options(c, optarg);
		if (err > 0) {
			continue;
		} else if (err < 0) {
			common_help(argc, argv);
			p_exit(1);
		}
		/* check for common options */
		switch (c) {
#ifdef USE_FTDI
		case 'V':
			i = (int)strtol(optarg, NULL, 16);
			if (errno == ERANGE || i < 0 || i > 0xffff) {
				ERROR_MSG("invalid usb vid value");
				p_exit(1);
			}
			usb_vid = (uint16_t)i;
			break;
		case 'P':
			i = (int)strtol(optarg, NULL, 16);
			if (errno == ERANGE || i < 0 || i > 0xffff) {
				ERROR_MSG("invalid usb pid value");
				p_exit(1);
			}
			usb_pid = (uint16_t)i;
			break;
		case 'D':
			usb_description = strdup(optarg);
			break;
		case 'S':
			usb_serial = strdup(optarg);
			break;
		case 'I':
			interface = atoi(optarg);
			if (interface < 0 || interface > 5) {
				ERROR_MSG("invalid interface");
				p_exit(1);
			}
			break;
		case 'R':
			reset = 1;
			break;
		case 'L':
			common_ftdi_list_print();
			p_exit(1);
#endif
		default:
		case '?':
		case 'h':
			common_help(argc, argv);
			p_exit(1);
		}
	}

	return 0;
}

#ifdef USE_FTDI

void common_ftdi_list_print()
{
	int i, n;
	struct ftdi_context *ftdi = NULL;
	struct ftdi_device_list *list;

	/* initialize ftdi */
	ftdi = ftdi_new();
	if (!ftdi) {
		fprintf(stderr, "ftdi_new() failed\n");
		return;
	}
	n = ftdi_usb_find_all(ftdi, &list, usb_vid, usb_pid);
	if (n < 1) {
		fprintf(stderr, "unable to find any matching device\n");
		return;
	}
	printf(LDC_DGRAYB"Matching USB devices (%04x:%04x):"LDC_DEFAULT"\n", usb_vid, usb_pid);
	for (i = 0; i < n; i++) {
		char m[1024], d[1024], s[1024];
		memset(m, 0, 1024);
		memset(d, 0, 1024);
		memset(s, 0, 1024);
		ftdi_usb_get_strings(ftdi, list->dev, m, 1024, d, 1024, s, 1024);
		list = list->next;
		if (usb_description) {
			if (strcmp(usb_description, d) != 0) {
				continue;
			}
		}
		if (usb_serial) {
			if (strcmp(usb_serial, s) != 0) {
				continue;
			}
		}
		printf(LDC_REDB"%10s "LDC_CYAN": %s / %s"LDC_DEFAULT"\n", s, d, m);
	}

	ftdi_list_free(&list);
	ftdi_free(ftdi);
}


struct ftdi_context *common_ftdi_init(void)
{
	if (interface >= 5) {
		return NULL;
	}
	return ftdi_open(usb_vid, usb_pid, interface, usb_description, usb_serial, reset);
}

#endif
