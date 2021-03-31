/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __BACKPORT_I2C_H
#define __BACKPORT_I2C_H

#include <linux/version.h>
#include_next <linux/i2c.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 7, 0)

/* I2C Frequency Modes */
#define I2C_MAX_STANDARD_MODE_FREQ     100000
#define I2C_MAX_FAST_MODE_FREQ         400000
#define I2C_MAX_FAST_MODE_PLUS_FREQ    1000000
#define I2C_MAX_TURBO_MODE_FREQ                1400000
#define I2C_MAX_HIGH_SPEED_MODE_FREQ   3400000
#define I2C_MAX_ULTRA_FAST_MODE_FREQ   5000000

#endif /* < KERNEL_VERSION(5, 7, 0) */

#endif
