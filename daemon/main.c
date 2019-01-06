/*
 * Gamepad daemon
 *
 * Authors:
 *  Antti Partanen <aehparta@iki.fi>
 */

#include <lib/debug.h>
#include <lib/packet.h>
#include <lib/comm.h>
#include <lib/os.h>
#include "gdd.h"


static const char opts[] = COMMON_SHORT_OPTS "o:";
static struct option longopts[] = {
	COMMON_LONG_OPTS
	{ 0, 0, 0, 0 },
};

int p_options(int c, char *optarg)
{
	switch (c) {
	}
	return 0;
}

void p_help(void)
{
	printf(
	    "\n"
	    "Gamepad daemon that creates input devices for controller devices found from network.\n"
	    "\n");
}

void sig_catch_int(int signum)
{
	signal(signum, sig_catch_int);
	INFO_MSG("SIGINT/SIGTERM (CTRL-C?) caught, exit application");
	p_exit(EXIT_FAILURE);
}

void sig_catch_tstp(int signum)
{
	signal(signum, sig_catch_tstp);
	WARN_MSG("SIGTSTP (CTRL-Z?) caught, don't do that");
}

void p_exit(int return_code)
{
	static int c = 0;
	c++;
	if (c > 1) {
		exit(return_code);
	}
	gdd_quit();
	comm_quit();
	log_quit();
	os_quit();
	exit(return_code);
}

int p_init(int argc, char *argv[])
{
	/* very low level platform initialization */
	os_init();
	/* debug/log init */
	log_init(NULL, 0);

	/* parse command line options */
	if (common_options(argc, argv, opts, longopts)) {
		ERROR_MSG("invalid command line option(s)");
		p_exit(EXIT_FAILURE);
	}
	/* signal handlers */
	signal(SIGINT, sig_catch_int);
	signal(SIGTERM, sig_catch_int);
	signal(SIGTSTP, sig_catch_tstp);

	/* communication initialization */
	ERROR_IF_R(comm_init(), -1, "communication link failed to initialize");

	gdd_init();

	return 0;
}

int main(int argc, char *argv[])
{
	uint32_t message_ids[32];
	int messagebuffer_cursor = 0;

	/* init */
	if (p_init(argc, argv)) {
		ERROR_MSG("initialization failed");
		p_exit(EXIT_FAILURE);
	}
	memset(message_ids, 0, sizeof(message_ids));

	/* program loop */
	DEBUG_MSG("starting main program loop");
	while (1) {
		struct packet pck;
		int ok = comm_recv(&pck, sizeof(pck));
		if (ok < 0) {
			CRIT_MSG("device disconnected?");
			break;
		} else if (ok > 0 && pck.mode == PACKET_MODE_BROADCAST && pck.type == PACKET_TYPE_GAMEPAD) {
			int found = 0;
			for (int i = 0; i < 32; i++) {
				if (PTOH24(pck.rid) == message_ids[i]) {
					found = 1;
					break;
				}
			}
			if (found) {
				continue;
			}
			message_ids[messagebuffer_cursor] = PTOH24(pck.rid);
			messagebuffer_cursor = (messagebuffer_cursor + 1) % 32;

			struct gdd *gdd = gdd_create(PTOH24(pck.from), 0);
			if (gdd) {
				gdd_set_buttons(gdd, pck.gamepad.buttons[0]);
			}

			/* data received */
			printf("%s%6ld -> %6ld, fw: %6ld, mode: %d, type: %3d, id: %10ld, buttons: %04x%s\r\n",
			       pck.mode == 2 ? LDC_WHITEB LDC_BBLUE : LDC_WHITEB LDC_BCYAN,
			       (long int)PTOH24(pck.from), (long int)PTOH24(pck.to),
			       (long int)PTOH24(pck.fw),
			       (int)pck.mode, (int)pck.type,
			       (long int)PTOH24(pck.rid),
			       pck.gamepad.buttons[0],
			       LDC_DEFAULT);
		}

		/* lets not waste all cpu */
		os_sleepf(0.001);
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}
