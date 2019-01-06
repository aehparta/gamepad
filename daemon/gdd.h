/*
 * Gamepad daemon device
 *
 * Authors:
 *  Antti Partanen <aehparta@iki.fi>
 */

#ifndef _GDD_H_
#define _GDD_H_

#include <stdint.h>

struct gdd {
	uint32_t id;
	int fd;

	struct gdd *prev;
	struct gdd *next;
};

int gdd_init(void);
void gdd_quit(void);

struct gdd *gdd_create(uint32_t id, uint8_t type);
void gdd_destroy(struct gdd *gdd);

int gdd_set_buttons(struct gdd *gdd, uint16_t buttons);

#endif /* _GDD_H_ */
