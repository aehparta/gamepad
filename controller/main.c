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
#include <libe/log.h>
#include <libe/drivers/misc/broadcast.h>
#include <libe/drivers/spi/nrf.h>
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
#ifdef USE_SPI
	nrf_disable_radio(&nrf);
	spi_master_close(&master);
#endif
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

	os_gpio_output(18);
	// os_gpio_high(18);
	// while(1);
	os_gpio_input(17);
	while (1) {
		if (os_gpio_read(17)) {
			os_gpio_low(18);
		} else {
			os_gpio_high(18);
		}
		// os_delay_ms(300);
	}

	/* initialize spi master */
#ifdef USE_FTDI
	ERROR_IF_R(common_ftdi_init(), -1, "need to have nrf device connected to ftdi");
#endif

#ifdef USE_SPI
	ERROR_IF_R(spi_master_open(
	               &master, /* must give pre-allocated spi master as pointer */
	               CFG_SPI_CONTEXT, /* context depends on platform */
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
#endif

	/* initialize broadcast */
#ifdef USE_BROADCAST
	ERROR_IF_R(broadcast_init(0), -1, "broadcast failed to initialize");
#endif

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
	os_gpio_output(GPIO_NES_CLOCK); /* clock */
	os_gpio_low(GPIO_NES_CLOCK);
	os_gpio_output(GPIO_NES_LATCH); /* latch */
	os_gpio_low(GPIO_NES_LATCH);
	os_gpio_input(GPIO_NES_INPUT); /* data */

	// while (1) {
	// 	os_gpio_high(GPIO_NES_CLOCK);
	// 	os_gpio_high(GPIO_NES_LATCH);
	// 	os_delay_ms(300);
	// 	os_gpio_low(GPIO_NES_CLOCK);
	// 	os_gpio_low(GPIO_NES_LATCH);
	// 	os_delay_ms(300);
	// }

	/* start program loop */
	INFO_MSG("starting program loop");
	while (1) {
		static uint16_t b_prev = 0xffff;
		uint16_t b = 0xff00;

		/* read nes, latch pulse first */
		os_gpio_high(GPIO_NES_LATCH);
		os_delay_us(10);
		os_gpio_low(GPIO_NES_LATCH);
		os_delay_us(10);

		for (int i = 0; i < 8; i++) {
			/* read button state */
			b |= os_gpio_read(GPIO_NES_INPUT) << i;
			/* clock pulse */
			os_gpio_high(GPIO_NES_CLOCK);
			os_delay_us(10);
			os_gpio_low(GPIO_NES_CLOCK);
			os_delay_us(10);
		}
		b = ~b;
		printf("buttons: %02x\r\n", b);

		/* send only if changed */
		if (b != b_prev) {
			struct gamepad_packet pck;
			memcpy(pck.magic, "gamepad\0", 8);
			pck.button = b;
			// pck.gamepad.buttons[1] = 0xffff;
			// pck.gamepad.buttons[2] = 0xffff;
			// pck.gamepad.buttons[3] = 0xffff;
// #ifdef USE_SPI
// 			nrf_send(&nrf, &pck);
// 			os_delay_us(100);
// 			nrf_send(&nrf, &pck);
// #endif
			b_prev = b;
			printf("buttons changed: %02x\r\n", b);
		}
	}

	return EXIT_SUCCESS;
}

