/*
 * Gamepad daemon
 *
 * Authors:
 *  Antti Partanen <aehparta@iki.fi>
 */

#include <libe/debug.h>
#include <libe/nrf.h>
#include <libe/os.h>
#include "gdd.h"
#include "cmd.h"
#include "config.h"


struct spi_master master;
struct nrf_device nrf;

static const char opts[] = COMMON_SHORT_OPTS;
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
	nrf_disable_radio(&nrf);
	gdd_quit();
	spi_master_close(&master);
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

	/* initialize spi master */
#ifdef USE_FTDI
	/* open ft232h type device and try to see if it has a nrf24l01+ connected to it through mpsse-spi */
	struct ftdi_context *context = common_ftdi_init();
#else
	void *context = CFG_SPI_CONTEXT;
#endif
	ERROR_IF_R(spi_master_open(
	               &master, /* must give pre-allocated spi master as pointer */
	               context, /* context depends on platform */
	               CFG_SPI_FREQUENCY,
	               CFG_SPI_MISO,
	               CFG_SPI_MOSI,
	               CFG_SPI_SCLK
	           ), -1, "failed to open spi master");

	/* nrf initialization */
	ERROR_IF_R(nrf_open(&nrf, &master, CFG_NRF_SS, CFG_NRF_CE), -1, "nrf24l01+ failed to initialize");
	/* change channel, default is 70 */
	nrf_set_channel(&nrf, 17);
	/* change speed, default is 250k */
	nrf_set_speed(&nrf, NRF_SPEED_2M);
	/* enable radio in listen mode */
	nrf_mode_rx(&nrf);
	nrf_flush_rx(&nrf);
	nrf_enable_radio(&nrf);

	/* gamepad daemon devices */
	gdd_init();

	return 0;
}

int main(int argc, char *argv[])
{
	/* init */
	if (p_init(argc, argv)) {
		ERROR_MSG("initialization failed");
		p_exit(EXIT_FAILURE);
	}

	/* program loop */
	INFO_MSG("starting main program loop");
	while (1) {

		// struct packet pck;
		// int ok = comm_recv(&pck, sizeof(pck));
		// if (ok < 0) {
		// 	CRIT_MSG("device disconnected?");
		// 	break;
		// } else if (ok > 0 && pck.mode == PACKET_MODE_BROADCAST && pck.type == PACKET_TYPE_GAMEPAD) {
		// 	int found = 0;
		// 	for (int i = 0; i < 32; i++) {
		// 		if (PTOH24(pck.rid) == message_ids[i]) {
		// 			found = 1;
		// 			break;
		// 		}
		// 	}
		// 	if (found) {
		// 		continue;
		// 	}
		// 	message_ids[messagebuffer_cursor] = PTOH24(pck.rid);
		// 	messagebuffer_cursor = (messagebuffer_cursor + 1) % 32;

		// 	struct gdd *gdd = gdd_create(PTOH24(pck.from), 0);
		// 	if (gdd) {
		// 		gdd_set_buttons(gdd, pck.gamepad.buttons[0]);
		// 	}

		// 	/* data received */
		// 	printf("%s%6ld -> %6ld, fw: %6ld, mode: %d, type: %3d, id: %10ld, buttons: %04x%s\r\n",
		// 	       pck.mode == 2 ? LDC_WHITEB LDC_BBLUE : LDC_WHITEB LDC_BCYAN,
		// 	       (long int)PTOH24(pck.from), (long int)PTOH24(pck.to),
		// 	       (long int)PTOH24(pck.fw),
		// 	       (int)pck.mode, (int)pck.type,
		// 	       (long int)PTOH24(pck.rid),
		// 	       pck.gamepad.buttons[0],
		// 	       LDC_DEFAULT);
		// }

		/* lets not waste all cpu */
		os_sleepf(0.001);
	}

	p_exit(EXIT_SUCCESS);
	return EXIT_SUCCESS;
}
