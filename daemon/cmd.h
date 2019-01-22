/*
 * Gamepad daemon device
 *
 * License: MIT
 * Authors: Antti Partanen <aehparta@iki.fi>
 */

#ifndef _CMD_COMMON_H__
#define _CMD_COMMON_H__

#include <getopt.h>
#ifdef USE_FTDI
#include <libftdi1/ftdi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


#ifdef USE_FTDI
#define COMMON_SHORT_OPTS "hV:P:D:S:I:RL"
#define COMMON_LONG_OPTS \
    { "help", no_argument, NULL, 'h' }, \
    { "vid", required_argument, NULL, 'V' }, \
    { "pid", required_argument, NULL, 'P' }, \
    { "description", required_argument, NULL, 'D' }, \
    { "serial", required_argument, NULL, 'S' }, \
    { "interface", required_argument, NULL, 'I' }, \
    { "reset", no_argument, NULL, 'R' }, \
    { "list-devices", no_argument, NULL, 'L' },
#else
#define COMMON_SHORT_OPTS "h"
#define COMMON_LONG_OPTS \
    { "help", no_argument, NULL, 'h' },
#endif


/**
 * Command specific option parsing.
 *
 * @param  c        option character
 * @param  optarg   optional argument
 * @return          1 if match, 0 if not
 */
int p_options(int c, char *optarg);

/**
 * Command specific exit.
 */
void p_exit(int return_code);

/**
 * Command specific help.
 */
void p_help(void);

/**
 * Print common help.
 */
void common_help(int argc, char *argv[]);

/**
 * Parse common options.
 */
int common_options(int argc, char *argv[], const char opts[], struct option longopts[]);

/**
 * List matching ftdi devices.
 */
void common_ftdi_list_print(void);

/**
 * Initialize ftdi resources.
 */
int common_ftdi_init(void);

int common_broadcast_init(void);


#ifdef __cplusplus
}
#endif

#endif /* __CMD_COMMON_H__ */
