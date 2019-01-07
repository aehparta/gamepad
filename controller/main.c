/*
 * Kotivo device: keypad remote controller
 *
 * Authors:
 *  Antti Partanen <aehparta@iki.fi>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <libe/os.h>
#include <libe/debug.h>
#include <libe/nrf.h>
#ifdef TARGET_ESP32
#include <freertos/task.h>
#endif
#include "../gamepad.h"
#include "../config.h"


struct spi_master master;
struct nrf_device nrf;


void p_exit(int return_code)
{
	static int c = 0;
	c++;
	if (c > 1) {
		exit(return_code);
	}
	nrf_disable_radio(&nrf);
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

	/* initialize spi master */
#ifdef USE_FTDI
	/* open ft232h type device and try to see if it has a nrf24l01+ connected to it through mpsse-spi */
	struct ftdi_context *context = ftdi_open(0x0403, 0x6014, 0, NULL, NULL, 1);
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

	return 0;
}

#ifdef TARGET_ESP32
int app_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	/* init */
	if (p_init(argc, argv)) {
		ERROR_MSG("initialization failed");
		p_exit(EXIT_FAILURE);
	}

	/* init nes controller */
	os_gpio_output(15); /* clock */
	os_gpio_low(15);
	os_gpio_output(2); /* latch */
	os_gpio_low(2);
	os_gpio_input(4); /* data */
	gpio_set_pull_mode(4, GPIO_PULLUP_ONLY);

	/* start program loop */
	INFO_MSG("starting program loop");
	while (1) {
		static uint16_t b_prev = 0xffff;
		uint16_t b = 0xff00;

		/* read nes, latch pulse first */
		os_gpio_high(2);
		os_sleepf(0.001);
		os_gpio_low(2);
		os_sleepf(0.001);

		for (int i = 0; i < 8; i++) {
			/* read button state */
			b |= os_gpio_read(4) ? (1 << i) : 0;
			/* clock pulse */
			os_gpio_high(15);
			os_gpio_low(15);
		}
		b = ~b;

		// DEBUG_MSG("buttons: %02x, %s", b, sp);

		if (b != b_prev) {
			struct gamepad_packet pck;
			memcpy(pck.magic, "gamepad\0", 8);
			pck.button = b;
			// pck.gamepad.buttons[1] = 0xffff;
			// pck.gamepad.buttons[2] = 0xffff;
			// pck.gamepad.buttons[3] = 0xffff;
			nrf_send(&nrf, &pck);
			b_prev = b;
			INFO_MSG("buttons changed: %02x", b);
		}

		os_sleepf(0.001);
	}

	return EXIT_SUCCESS;
}

