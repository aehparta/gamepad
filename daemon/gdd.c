/*
 * Gamepad daemon device
 *
 * Authors:
 *  Antti Partanen <aehparta@iki.fi>
 */

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <linux/uinput.h>
#include <libe/debug.h>
#include <libe/linkedlist.h>
#include "gdd.h"


static struct gdd *gdd_first;
static struct gdd *gdd_last;


int gdd_init(void)
{
	return 0;
}

void gdd_quit(void)
{
}

struct gdd *gdd_create(uint32_t id, uint8_t type)
{
	struct gdd *gdd;
	struct uinput_setup usetup;
	int fd;

	for (gdd = gdd_first; gdd; gdd = gdd->next) {
		if (gdd->id == id) {
			return gdd;
		}
	}

	fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
	ERROR_IF_R(fd < 0, NULL, "failed to create new uinput device");

	/* enable the device */
	ioctl(fd, UI_SET_EVBIT, EV_KEY);
	ioctl(fd, UI_SET_KEYBIT, BTN_A);
	ioctl(fd, UI_SET_KEYBIT, BTN_B);
	ioctl(fd, UI_SET_KEYBIT, BTN_SELECT);
	ioctl(fd, UI_SET_KEYBIT, BTN_START);
	ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_UP);
	ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_DOWN);
	ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_LEFT);
	ioctl(fd, UI_SET_KEYBIT, BTN_DPAD_RIGHT);

	/* setup and create */
	memset(&usetup, 0, sizeof(usetup));
	usetup.id.bustype = BUS_USB;
	usetup.id.vendor = 0x7777;
	usetup.id.product = 0x7777;
	strcpy(usetup.name, "Duge's gamepad");
	ioctl(fd, UI_DEV_SETUP, &usetup);
	ioctl(fd, UI_DEV_CREATE);

	/* allocate device and fill data */
	SALLOC(gdd, NULL);
	gdd->id = id;
	gdd->fd = fd;
	LL_APP(gdd_first, gdd_last, gdd);

	return gdd;
}

void gdd_destroy(struct gdd *gdd)
{
	if (gdd) {
		ioctl(gdd->fd, UI_DEV_DESTROY);
		close(gdd->fd);
		free(gdd);
	}
}

int gdd_set_buttons(struct gdd *gdd, uint16_t buttons)
{
	struct input_event ie;
	ie.time.tv_sec = 0;
	ie.time.tv_usec = 0;

	for (int i = 0; i < 8; i++) {
		ie.type = EV_KEY;
		ie.code = 0;
		ie.value = buttons & (1 << i) ? 1 : 0;

		switch (i) {
		case 0:
			ie.code = BTN_A;
			break;
		case 1:
			ie.code = BTN_B;
			break;
		case 2:
			ie.code = BTN_SELECT;
			break;
		case 3:
			ie.code = BTN_START;
			break;
		case 4:
			ie.code = BTN_DPAD_UP;
			break;
		case 5:
			ie.code = BTN_DPAD_DOWN;
			break;
		case 6:
			ie.code = BTN_DPAD_LEFT;
			break;
		case 7:
			ie.code = BTN_DPAD_RIGHT;
			break;
		}

		ERROR_IF(write(gdd->fd, &ie, sizeof(ie)) < 1, "write failed");
	}

	ie.type = EV_SYN;
	ie.code = SYN_REPORT;
	ie.value = 0;
	ERROR_IF(write(gdd->fd, &ie, sizeof(ie)) < 1, "write failed");

	return 0;
}

