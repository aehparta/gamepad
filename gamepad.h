
#ifndef _GAMEPAD_H_
#define _GAMEPAD_H_

#include <stdint.h>

struct gamepad_packet {
	uint8_t magic[8];
	uint16_t button;
	uint8_t pad[22];
};

#endif /* _GAMEPAD_H_ */
