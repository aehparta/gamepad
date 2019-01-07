
/*
 * Platform specific configuration defaults.
 */

#ifdef TARGET_X86
#define CFG_SPI_CONTEXT     NULL
#define CFG_SPI_FREQUENCY   0
#define CFG_SPI_MISO        0
#define CFG_SPI_MOSI        0
#define CFG_SPI_SCLK        0
#define CFG_NRF_SS          0x08
#define CFG_NRF_CE          0x10
#endif

#ifdef TARGET_RPI
#define CFG_SPI_CONTEXT     NULL
#define CFG_SPI_FREQUENCY   0
#define CFG_SPI_MISO        0
#define CFG_SPI_MOSI        0
#define CFG_SPI_SCLK        0
#define CFG_NRF_SS          8
#define CFG_NRF_CE          25
#endif

#ifdef TARGET_ESP32
#define CFG_SPI_CONTEXT     ((void *)HSPI_HOST)
#define CFG_SPI_FREQUENCY   0
#define CFG_SPI_MISO        GPIO_NUM_12
#define CFG_SPI_MOSI        GPIO_NUM_13
#define CFG_SPI_SCLK        GPIO_NUM_14
#define CFG_NRF_SS          GPIO_NUM_26
#define CFG_NRF_CE          GPIO_NUM_27
#endif

